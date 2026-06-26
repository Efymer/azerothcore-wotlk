/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
* @file main.cpp
* @brief Battle.net Server main program
*
* This file contains the main program for the
* Battle.net authentication server
*/

#include "AppenderDB.h"
#include "Banner.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "GitRevision.h"
#include "IoContext.h"
#include "IPLocation.h"
#include "Log.h"
#include "LoginRESTService.h"
#include "Memory.h"
#include "MySQLThreading.h"
#include "SRP6.h"
#include "OpenSSLCrypto.h"
#include "ProcessPriority.h"
#include "RealmList.h"
#include "SecretMgr.h"
#include "SessionManager.h"
#include "SharedDefines.h"
#include "SslContext.h"
#include "SteadyTimer.h"
#include "StringFormat.h"
#include "Util.h"
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/version.hpp>
#include <google/protobuf/stubs/common.h>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <openssl/crypto.h>
#include <openssl/opensslv.h>

#ifndef _ACORE_BNET_CONFIG
#define _ACORE_BNET_CONFIG "bnetserver.conf"
#endif

using boost::asio::ip::tcp;
using namespace boost::program_options;
namespace fs = std::filesystem;

bool StartDB();
void StopDB();
int RegisterBnetAccount(std::string email, std::string const& password);
void SignalHandler(std::weak_ptr<Acore::Asio::IoContext> ioContextRef, boost::system::error_code const& error, int signalNumber);
void KeepDatabaseAliveHandler(std::weak_ptr<boost::asio::steady_timer> dbPingTimerRef, int32 dbPingInterval, boost::system::error_code const& error);
void BanExpiryHandler(std::weak_ptr<boost::asio::steady_timer> banExpiryCheckTimerRef, int32 banExpiryCheckInterval, boost::system::error_code const& error);
variables_map GetConsoleArguments(int argc, char** argv, fs::path& configFile);

/// Launch the Battle.net server
int main(int argc, char** argv)
{
    Acore::Impl::CurrentServerProcessHolder::_type = SERVER_PROCESS_AUTHSERVER;
    signal(SIGABRT, &Acore::AbortHandler);

    // Command line parsing
    auto configFile = fs::path(sConfigMgr->GetConfigPath() + std::string(_ACORE_BNET_CONFIG));
    auto vm = GetConsoleArguments(argc, argv, configFile);

    // exit if help or version is enabled
    if (vm.count("help") || vm.count("version"))
        return 0;

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::shared_ptr<void> protobufHandle(nullptr, [](void*) { google::protobuf::ShutdownProtobufLibrary(); });

    // Add file and args in config
    sConfigMgr->Configure(configFile.generic_string(), std::vector<std::string>(argv, argv + argc));

    if (!sConfigMgr->LoadAppConfigs())
        return 1;

    // Init logging
    sLog->RegisterAppender<AppenderDB>();
    sLog->Initialize(nullptr);

    Acore::Banner::Show("bnetserver",
        [](std::string_view text)
        {
            LOG_INFO("server.bnetserver", text);
        },
        []()
        {
            LOG_INFO("server.bnetserver", "> Using configuration file       {}", sConfigMgr->GetFilename());
            LOG_INFO("server.bnetserver", "> Using SSL version:             {} (library: {})", OPENSSL_VERSION_TEXT, OpenSSL_version(OPENSSL_VERSION));
            LOG_INFO("server.bnetserver", "> Using Boost version:           {}.{}.{}", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);
        });

    OpenSSLCrypto::threadsSetup();

    std::shared_ptr<void> opensslHandle(nullptr, [](void*) { OpenSSLCrypto::threadsCleanup(); });

    // bnetserver PID file creation
    std::string pidFile = sConfigMgr->GetOption<std::string>("PidFile", "");
    if (!pidFile.empty())
    {
        if (uint32 pid = CreatePIDFile(pidFile))
            LOG_INFO("server.bnetserver", "Daemon PID: {}\n", pid); // outError for red color in console
        else
        {
            LOG_ERROR("server.bnetserver", "Cannot create PID file {} (possible error: permission)\n", pidFile);
            return 1;
        }
    }

    if (!Battlenet::SslContext::Initialize())
    {
        LOG_ERROR("server.bnetserver", "Failed to initialize SSL context");
        return 1;
    }

    // Initialize the database connection
    if (!StartDB())
        return 1;

    std::shared_ptr<void> dbHandle(nullptr, [](void*) { StopDB(); });

    // CLI account-creation mode: register a battle.net account and exit without
    // starting the REST/bnet network listeners.
    if (vm.count("register-bnet"))
    {
        std::vector<std::string> const args = vm["register-bnet"].as<std::vector<std::string>>();

        std::string email;
        std::string password;

        if (args.size() == 1)
        {
            // Allow the <email>:<password> single-argument form.
            std::string const& combined = args[0];
            std::size_t const sep = combined.find(':');
            if (sep == std::string::npos)
            {
                LOG_ERROR("server.bnetserver", "--register-bnet: expected <email> <password> or <email>:<password>");
                return 1;
            }

            email = combined.substr(0, sep);
            password = combined.substr(sep + 1);
        }
        else if (args.size() >= 2)
        {
            email = args[0];
            password = args[1];
        }
        else
        {
            LOG_ERROR("server.bnetserver", "--register-bnet: expected <email> <password> or <email>:<password>");
            return 1;
        }

        if (email.empty() || password.empty())
        {
            LOG_ERROR("server.bnetserver", "--register-bnet: email and password must not be empty");
            return 1;
        }

        return RegisterBnetAccount(std::move(email), password);
    }

    sSecretMgr->Initialize(SECRET_OWNER_BNETSERVER);

    // Load IP Location Database
    sIPLocation->Load();

    std::shared_ptr<Acore::Asio::IoContext> ioContext = std::make_shared<Acore::Asio::IoContext>();

    std::string httpBindIp = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");
    int32 httpPort = sConfigMgr->GetOption<int32>("LoginREST.Port", 8081);
    if (httpPort <= 0 || httpPort > 0xFFFF)
    {
        LOG_ERROR("server.bnetserver", "Specified login service port ({}) out of allowed range (1-65535)", httpPort);
        return 1;
    }

    if (!sLoginService.StartNetwork(*ioContext, httpBindIp, httpPort))
    {
        LOG_ERROR("server.bnetserver", "Failed to initialize login service");
        return 1;
    }

    std::shared_ptr<void> sLoginServiceHandle(nullptr, [](void*) { sLoginService.StopNetwork(); });

    // Start the listening port (acceptor) for auth connections
    int32 bnport = sConfigMgr->GetOption<int32>("BattlenetPort", 1119);
    if (bnport <= 0 || bnport > 0xFFFF)
    {
        LOG_ERROR("server.bnetserver", "Specified battle.net port ({}) out of allowed range (1-65535)", bnport);
        return 1;
    }

    // Get the list of realms for the server
    sRealmList->Initialize(*ioContext, sConfigMgr->GetOption<int32>("RealmsStateUpdateDelay", 10));

    std::shared_ptr<void> sRealmListHandle(nullptr, [](void*) { sRealmList->Close(); });

    std::string bindIp = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");

    if (!sSessionMgr.StartNetwork(*ioContext, bindIp, bnport))
    {
        LOG_ERROR("server.bnetserver", "Failed to initialize network");
        return 1;
    }

    std::shared_ptr<void> sSessionMgrHandle(nullptr, [](void*) { sSessionMgr.StopNetwork(); });

    // Set signal handlers
    boost::asio::signal_set signals(*ioContext, SIGINT, SIGTERM);
#if AC_PLATFORM == AC_PLATFORM_WINDOWS
    signals.add(SIGBREAK);
#endif
    signals.async_wait(std::bind(&SignalHandler, std::weak_ptr<Acore::Asio::IoContext>(ioContext), std::placeholders::_1, std::placeholders::_2));

    // Set process priority according to configuration settings
    SetProcessPriority("server.bnetserver", sConfigMgr->GetOption<int32>(CONFIG_PROCESSOR_AFFINITY, 0), sConfigMgr->GetOption<bool>(CONFIG_HIGH_PRIORITY, false));

    // Enabled a timed callback for handling the database keep alive ping
    int32 dbPingInterval = sConfigMgr->GetOption<int32>("MaxPingTime", 30);
    std::shared_ptr<boost::asio::steady_timer> dbPingTimer = std::make_shared<boost::asio::steady_timer>(*ioContext);

    dbPingTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(dbPingInterval * MINUTE));
    dbPingTimer->async_wait(std::bind(&KeepDatabaseAliveHandler, std::weak_ptr<boost::asio::steady_timer>(dbPingTimer), dbPingInterval, std::placeholders::_1));

    int32 banExpiryCheckInterval = sConfigMgr->GetOption<int32>("BanExpiryCheckInterval", 60);
    std::shared_ptr<boost::asio::steady_timer> banExpiryCheckTimer = std::make_shared<boost::asio::steady_timer>(*ioContext);

    banExpiryCheckTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(banExpiryCheckInterval));
    banExpiryCheckTimer->async_wait(std::bind(&BanExpiryHandler, std::weak_ptr<boost::asio::steady_timer>(banExpiryCheckTimer), banExpiryCheckInterval, std::placeholders::_1));

    // Start the io service worker loop
    ioContext->run();

    banExpiryCheckTimer->cancel();
    dbPingTimer->cancel();

    LOG_INFO("server.bnetserver", "Halting process...");

    signals.cancel();

    return 0;
}

/// Initialize connection to the database
bool StartDB()
{
    MySQL::Library_Init();

    // Load databases
    DatabaseLoader loader("server.bnetserver");
    loader
        .AddDatabase(LoginDatabase, "Login");

    if (!loader.Load())
        return false;

    LOG_INFO("server.bnetserver", "Started auth database connection pool.");
    sLog->SetRealmId(0); // Enables DB appenders when realm is set.
    return true;
}

/// Close the connection to the database
void StopDB()
{
    LoginDatabase.Close();
    MySQL::Library_End();
}

/// Create a battle.net account (and a linked grunt game account) from the CLI, then exit.
/// Mirrors TrinityCore's Battlenet::AccountMgr::CreateBattlenetAccount using AzerothCore's API.
int RegisterBnetAccount(std::string email, std::string const& password)
{
    // The battle.net SRP username is the hex-encoded SHA-256 of the uppercased
    // email — this MUST match how LoginRESTService derives it at login time
    // (HexStr(SHA256(account_name))), otherwise the verifier never matches.
    Utf8ToUpperOnlyLatin(email);
    std::string const srpUsername = ByteArrayToHexStr(Acore::Crypto::SHA256::GetDigestOf(email));

    // Reject duplicates up front.
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_ID_BY_EMAIL);
    stmt->SetData(0, email);
    if (LoginDatabase.Query(stmt))
    {
        LOG_ERROR("server.bnetserver", "Battle.net account '{}' already exists.", email);
        return 1;
    }

    // Derive the v2 registration data (salt: 32-byte array, verifier: byte vector).
    // MakeRegistrationData is a static on the base; the concrete impl is the explicit template arg.
    using BnetSRP6 = Acore::Crypto::SRP::BnetSRP6v2<Acore::Crypto::SHA256>;
    auto [salt, verifier] = BnetSRP6::MakeRegistrationData<BnetSRP6>(srpUsername, password);

    // INSERT INTO battlenet_accounts (email, srp_version, salt, verifier).
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BNET_ACCOUNT);
    stmt->SetData(0, email);
    stmt->SetData(1, uint8(2)); // SrpVersion::v2
    stmt->SetData(2, salt);
    stmt->SetData(3, verifier);
    LoginDatabase.DirectExecute(stmt);

    // Fetch the newly created battle.net account id.
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_ID_BY_EMAIL);
    stmt->SetData(0, email);
    PreparedQueryResult result = LoginDatabase.Query(stmt);
    if (!result)
    {
        LOG_ERROR("server.bnetserver", "Failed to create battle.net account '{}' (could not read back id).", email);
        return 1;
    }

    uint32 const bnetAccountId = (*result)[0].Get<uint32>();

    // Create the linked grunt game account named "<bnetId>#1" so the realm list works.
    std::string gameAccountName = Acore::StringFormat("{}#1", bnetAccountId);

    std::string gruntUsername = gameAccountName;
    std::string gruntPassword = password;
    Utf8ToUpperOnlyLatin(gruntUsername);
    Utf8ToUpperOnlyLatin(gruntPassword);

    // Make sure the game account name is free.
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME);
    stmt->SetData(0, gruntUsername);
    if (LoginDatabase.Query(stmt))
    {
        LOG_ERROR("server.bnetserver", "Battle.net account '{}' created (id {}), but linked game account '{}' already exists.",
            email, bnetAccountId, gruntUsername);
        return 1;
    }

    // INSERT INTO account(username, salt, verifier, expansion, reg_mail, email) using the legacy grunt SRP6.
    auto [gruntSalt, gruntVerifier] = Acore::Crypto::SRP6::MakeRegistrationData(gruntUsername, gruntPassword);

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT);
    stmt->SetData(0, gruntUsername);
    stmt->SetData(1, gruntSalt);
    stmt->SetData(2, gruntVerifier);
    stmt->SetData(3, uint8(2)); // expansion (WotLK)
    stmt->SetData(4, ""); // reg_mail
    stmt->SetData(5, ""); // email
    LoginDatabase.DirectExecute(stmt);

    // Resolve the new game account id and link it to the battle.net account at index 1.
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME);
    stmt->SetData(0, gruntUsername);
    result = LoginDatabase.Query(stmt);
    if (!result)
    {
        LOG_ERROR("server.bnetserver", "Battle.net account '{}' created (id {}), but failed to read back game account '{}'.",
            email, bnetAccountId, gruntUsername);
        return 1;
    }

    uint32 const gameAccountId = (*result)[0].Get<uint32>();

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LINK);
    stmt->SetData(0, bnetAccountId);
    stmt->SetData(1, uint8(1)); // battlenet_index
    stmt->SetData(2, gameAccountId);
    LoginDatabase.DirectExecute(stmt);

    LOG_INFO("server.bnetserver", "Created battle.net account '{}' (id {}) with linked game account '{}' (id {}, index 1).",
        email, bnetAccountId, gruntUsername, gameAccountId);
    return 0;
}

void SignalHandler(std::weak_ptr<Acore::Asio::IoContext> ioContextRef, boost::system::error_code const& error, int /*signalNumber*/)
{
    if (!error)
    {
        if (std::shared_ptr<Acore::Asio::IoContext> ioContext = ioContextRef.lock())
        {
            ioContext->stop();
        }
    }
}

void KeepDatabaseAliveHandler(std::weak_ptr<boost::asio::steady_timer> dbPingTimerRef, int32 dbPingInterval, boost::system::error_code const& error)
{
    if (!error)
    {
        if (std::shared_ptr<boost::asio::steady_timer> dbPingTimer = dbPingTimerRef.lock())
        {
            LOG_DEBUG("sql.driver", "Ping MySQL to keep connection alive");
            LoginDatabase.KeepAlive();

            dbPingTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(dbPingInterval * MINUTE));
            dbPingTimer->async_wait(std::bind(&KeepDatabaseAliveHandler, dbPingTimerRef, dbPingInterval, std::placeholders::_1));
        }
    }
}

void BanExpiryHandler(std::weak_ptr<boost::asio::steady_timer> banExpiryCheckTimerRef, int32 banExpiryCheckInterval, boost::system::error_code const& error)
{
    if (!error)
    {
        if (std::shared_ptr<boost::asio::steady_timer> banExpiryCheckTimer = banExpiryCheckTimerRef.lock())
        {
            LoginDatabase.Execute(LoginDatabase.GetPreparedStatement(LOGIN_DEL_EXPIRED_IP_BANS));
            LoginDatabase.Execute(LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPIRED_ACCOUNT_BANS));
            LoginDatabase.Execute(LoginDatabase.GetPreparedStatement(LOGIN_DEL_BNET_EXPIRED_ACCOUNT_BANNED));

            banExpiryCheckTimer->expires_at(Acore::Asio::SteadyTimer::GetExpirationTime(banExpiryCheckInterval));
            banExpiryCheckTimer->async_wait(std::bind(&BanExpiryHandler, banExpiryCheckTimerRef, banExpiryCheckInterval, std::placeholders::_1));
        }
    }
}

variables_map GetConsoleArguments(int argc, char** argv, fs::path& configFile)
{
    options_description all("Allowed options");
    all.add_options()
        ("help,h", "print usage message")
        ("version,v", "print version build info")
        ("dry-run,d", "Dry run")
        ("config,c", value<fs::path>(&configFile)->default_value(fs::path(sConfigMgr->GetConfigPath() + std::string(_ACORE_BNET_CONFIG))), "use <arg> as configuration file")
        ("config-policy", value<std::string>()->value_name("policy"), "override config severity policy (e.g. default=skip,critical_option=fatal)")
        ("register-bnet", value<std::vector<std::string>>()->multitoken(), "create a battle.net account: --register-bnet <email> <password> (or <email>:<password>) then exit");

    variables_map variablesMap;

    try
    {
        store(command_line_parser(argc, argv).options(all).allow_unregistered().run(), variablesMap);
        notify(variablesMap);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << "\n";
    }

    if (variablesMap.count("help"))
        std::cout << all << "\n";
    else if (variablesMap.count("version"))
        std::cout << GitRevision::GetFullVersion() << "\n";
    else if (variablesMap.count("dry-run"))
        sConfigMgr->setDryRun(true);

    return variablesMap;
}
