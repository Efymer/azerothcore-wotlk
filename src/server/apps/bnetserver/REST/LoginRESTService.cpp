/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LoginRESTService.h"
#include "Base64.h"
#include "Config.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "IpNetwork.h"
#include "Log.h"
#include "ProtobufJSON.h"
#include "Resolver.h"
#include "SslContext.h"
#include "StringFormat.h"
#include "Timer.h"
#include "Util.h"

namespace Battlenet
{
LoginRESTService& LoginRESTService::Instance()
{
    static LoginRESTService instance;
    return instance;
}

bool LoginRESTService::StartNetwork(Acore::Asio::IoContext& ioContext, std::string const& bindIp, uint16 port, int32 threadCount)
{
    if (!HttpService::StartNetwork(ioContext, bindIp, port, threadCount))
        return false;

    using Acore::Net::Http::RequestHandlerFlag;

    RegisterHandler(boost::beast::http::verb::get, "/bnetserver/login/", [this](std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
    {
        return HandleGetForm(std::move(session), context);
    });

    RegisterHandler(boost::beast::http::verb::get, "/bnetserver/gameAccounts/", [](std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
    {
        return HandleGetGameAccounts(std::move(session), context);
    });

    RegisterHandler(boost::beast::http::verb::get, "/bnetserver/portal/", [this](std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
    {
        return HandleGetPortal(std::move(session), context);
    });

    RegisterHandler(boost::beast::http::verb::post, "/bnetserver/login/", [this](std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
    {
        return HandlePostLogin(std::move(session), context);
    }, RequestHandlerFlag::DoNotLogRequestContent);

    RegisterHandler(boost::beast::http::verb::post, "/bnetserver/login/srp/", [](std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
    {
        return HandlePostLoginSrpChallenge(std::move(session), context);
    });

    RegisterHandler(boost::beast::http::verb::post, "/bnetserver/refreshLoginTicket/", [this](std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
    {
        return HandlePostRefreshLoginTicket(std::move(session), context);
    });

    _bindIP = bindIp;
    _port = port;

    using namespace std::string_literals;
    std::array<std::string, 2> configKeys = { { "LoginREST.ExternalAddress"s, "LoginREST.LocalAddress"s } };

    Acore::Asio::Resolver resolver(ioContext);

    for (std::size_t i = 0; i < _hostnames.size(); ++i)
    {
        _hostnames[i].first = sConfigMgr->GetOption<std::string>(configKeys[i], "127.0.0.1");

        Optional<boost::asio::ip::tcp::endpoint> endpoint = resolver.Resolve(boost::asio::ip::tcp::v4(), _hostnames[i].first, "");
        if (!endpoint)
            endpoint = resolver.Resolve(boost::asio::ip::tcp::v6(), _hostnames[i].first, "");

        if (!endpoint)
        {
            LOG_ERROR("server.http.login", "Could not resolve {} {}", configKeys[i], _hostnames[i].first);
            return false;
        }

        _hostnames[i].second.push_back(endpoint->address());
    }

    // set up form inputs
    JSON::Login::FormInput* input;
    _formInputs.set_type(JSON::Login::LOGIN_FORM);
    input = _formInputs.add_inputs();
    input->set_input_id("account_name");
    input->set_type("text");
    input->set_label("E-mail");
    input->set_max_length(320);

    input = _formInputs.add_inputs();
    input->set_input_id("password");
    input->set_type("password");
    input->set_label("Password");
    input->set_max_length(128);

    input = _formInputs.add_inputs();
    input->set_input_id("log_in_submit");
    input->set_type("submit");
    input->set_label("Log In");

    _loginTicketDuration = sConfigMgr->GetOption<uint32>("LoginREST.TicketDuration", 3600);

    MigrateLegacyPasswordHashes();

    _acceptor->AsyncAccept([this](Acore::Net::IoContextTcpSocket&& sock, uint32 threadIndex)
    {
        OnSocketOpen(std::move(sock), threadIndex);
    });
    return true;
}

std::string const& LoginRESTService::GetHostnameForClient(boost::asio::ip::address const& address) const
{
    if (address.is_loopback())
        return _hostnames[1].first;

    if (address.is_v4())
    {
        // if the client is in the local subnet, send the local hostname
        for (boost::asio::ip::address const& localAddress : _hostnames[1].second)
        {
            if (localAddress.is_v4()
                && Acore::Net::IsInNetwork(localAddress.to_v4(), Acore::Net::GetDefaultNetmaskV4(localAddress.to_v4()), address.to_v4()))
            {
                return _hostnames[1].first;
            }
        }
    }

    return _hostnames[0].first;
}

std::string LoginRESTService::ExtractAuthorization(HttpRequest const& request)
{
    using namespace std::string_view_literals;

    std::string ticket;
    auto itr = request.find(boost::beast::http::field::authorization);
    if (itr == request.end())
        return ticket;

    std::string_view authorization = Acore::Net::Http::ToStdStringView(itr->value());
    constexpr std::string_view BASIC_PREFIX = "Basic "sv;

    if (authorization.starts_with(BASIC_PREFIX))
        authorization.remove_prefix(BASIC_PREFIX.length());

    Optional<std::vector<uint8>> decoded = Acore::Encoding::Base64::Decode(std::string(authorization));
    if (!decoded)
        return ticket;

    std::string_view decodedHeader(reinterpret_cast<char const*>(decoded->data()), decoded->size());

    if (std::size_t ticketEnd = decodedHeader.find(':'); ticketEnd != std::string_view::npos)
        decodedHeader.remove_suffix(decodedHeader.length() - ticketEnd);

    ticket = decodedHeader;
    return ticket;
}

LoginRESTService::RequestHandlerResult LoginRESTService::HandleGetForm(std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context) const
{
    JSON::Login::FormInputs form = _formInputs;
    form.set_srp_url(Acore::StringFormat("http{}://{}:{}/bnetserver/login/srp/", !SslContext::UsesDevWildcardCertificate() ? "s" : "",
        GetHostnameForClient(session->GetRemoteIpAddress()), _port));

    context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
    context.response.body() = ::JSON::Serialize(form);
    return RequestHandlerResult::Handled;
}

LoginRESTService::RequestHandlerResult LoginRESTService::HandleGetGameAccounts(std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
{
    std::string ticket = ExtractAuthorization(context.request);
    if (ticket.empty())
        return HandleUnauthorized(std::move(session), context);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_GAME_ACCOUNT_LIST);
    stmt->SetData(0, ticket);
    session->QueueQuery(LoginDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([session, context = std::move(context)](PreparedQueryResult result) mutable
    {
        JSON::Login::GameAccountList gameAccounts;
        if (result)
        {
            auto formatDisplayName = [](char const* name) -> std::string
            {
                if (char const* hashPos = strchr(name, '#'))
                    return std::string("WoW") + ++hashPos;
                else
                    return name;
            };

            time_t now = time(nullptr);
            do
            {
                Field* fields = result->Fetch();
                JSON::Login::GameAccountInfo* gameAccount = gameAccounts.add_game_accounts();
                gameAccount->set_display_name(formatDisplayName(fields[0].Get<std::string>().c_str()));
                gameAccount->set_expansion(fields[1].Get<uint8>());
                if (!fields[2].IsNull())
                {
                    uint32 banDate = fields[2].Get<uint32>();
                    uint32 unbanDate = fields[3].Get<uint32>();
                    gameAccount->set_is_suspended(unbanDate > now);
                    gameAccount->set_is_banned(banDate == unbanDate);
                    gameAccount->set_suspension_reason(fields[4].Get<std::string>());
                    gameAccount->set_suspension_expires(unbanDate);
                }
            } while (result->NextRow());
        }

        context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
        context.response.body() = ::JSON::Serialize(gameAccounts);
        session->SendResponse(context);
    }));

    return RequestHandlerResult::Async;
}

LoginRESTService::RequestHandlerResult LoginRESTService::HandleGetPortal(std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context) const
{
    context.response.set(boost::beast::http::field::content_type, "text/plain");
    context.response.body() = Acore::StringFormat("{}:{}", GetHostnameForClient(session->GetRemoteIpAddress()), sConfigMgr->GetOption<int32>("BattlenetPort", 1119));
    return RequestHandlerResult::Handled;
}

LoginRESTService::RequestHandlerResult LoginRESTService::HandlePostLogin(std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context) const
{
    std::shared_ptr<JSON::Login::LoginForm> loginForm = std::make_shared<JSON::Login::LoginForm>();
    if (!::JSON::Deserialize(context.request.body(), loginForm.get()))
    {
        JSON::Login::LoginResult loginResult;
        loginResult.set_authentication_state(JSON::Login::LOGIN);
        loginResult.set_error_code("UNABLE_TO_DECODE");
        loginResult.set_error_message("There was an internal error while connecting to Battle.net. Please try again later.");

        context.response.result(boost::beast::http::status::bad_request);
        context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
        context.response.body() = ::JSON::Serialize(loginResult);
        session->SendResponse(context);

        return RequestHandlerResult::Handled;
    }

    auto getInputValue = [](JSON::Login::LoginForm const* loginForm, std::string_view inputId) -> std::string
    {
        for (int32 i = 0; i < loginForm->inputs_size(); ++i)
            if (loginForm->inputs(i).input_id() == inputId)
                return loginForm->inputs(i).value();
        return "";
    };

    std::string login(getInputValue(loginForm.get(), "account_name"));
    Utf8ToUpperOnlyLatin(login);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_AUTHENTICATION);
    stmt->SetData(0, login);

    session->QueueQuery(LoginDatabase.AsyncQuery(stmt)
        .WithChainingPreparedCallback([this, session, context = std::move(context), loginForm = std::move(loginForm), getInputValue](QueryCallback& callback, PreparedQueryResult result) mutable
    {
        if (!result)
        {
            JSON::Login::LoginResult loginResult;
            loginResult.set_authentication_state(JSON::Login::DONE);
            context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
            context.response.body() = ::JSON::Serialize(loginResult);
            session->SendResponse(context);
            return;
        }

        std::string login(getInputValue(loginForm.get(), "account_name"));
        Utf8ToUpperOnlyLatin(login);
        bool passwordCorrect = false;
        Optional<std::string> serverM2;

        Field* fields = result->Fetch();
        uint32 accountId = fields[0].Get<uint32>();
        if (!session->GetSessionState()->Srp)
        {
            SrpVersion version = SrpVersion(fields[1].Get<int8>());
            std::string srpUsername = ByteArrayToHexStr(Acore::Crypto::SHA256::GetDigestOf(login));
            Acore::Crypto::SRP::Salt s = fields[2].Get<Binary, Acore::Crypto::SRP::SALT_LENGTH>();
            Acore::Crypto::SRP::Verifier v = fields[3].Get<Binary>();
            session->GetSessionState()->Srp = CreateSrpImplementation(version, SrpHashFunction::Sha256, srpUsername, s, v);

            std::string password(getInputValue(loginForm.get(), "password"));
            if (version == SrpVersion::v1)
                Utf8ToUpperOnlyLatin(password);

            passwordCorrect = session->GetSessionState()->Srp->CheckCredentials(srpUsername, password);
        }
        else
        {
            BigNumber A(getInputValue(loginForm.get(), "public_A"));
            BigNumber M1(getInputValue(loginForm.get(), "client_evidence_M1"));
            if (Optional<BigNumber> sessionKey = session->GetSessionState()->Srp->VerifyClientEvidence(A, M1))
            {
                passwordCorrect = true;
                serverM2 = session->GetSessionState()->Srp->CalculateServerEvidence(A, M1, *sessionKey).AsHexStr();
            }
        }

        uint32 failedLogins = fields[4].Get<uint32>();
        std::string loginTicket = fields[5].Get<std::string>();
        uint32 loginTicketExpiry = fields[6].Get<uint32>();
        bool isBanned = fields[7].Get<uint64>() != 0;

        if (!passwordCorrect)
        {
            if (!isBanned)
            {
                std::string ip_address = session->GetRemoteIpAddress().to_string();
                uint32 maxWrongPassword = uint32(sConfigMgr->GetOption<int32>("WrongPass.MaxCount", 0));

                if (sConfigMgr->GetOption<bool>("WrongPass.Logging", false))
                    LOG_DEBUG("server.http.login", "[{}, Account {}, Id {}] Attempted to connect with wrong password!", ip_address, login, accountId);

                if (maxWrongPassword)
                {
                    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();
                    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_FAILED_LOGINS);
                    stmt->SetData(0, accountId);
                    trans->Append(stmt);

                    ++failedLogins;

                    LOG_DEBUG("server.http.login", "MaxWrongPass : {}, failed_login : {}", maxWrongPassword, accountId);

                    if (failedLogins >= maxWrongPassword)
                    {
                        BanMode banType = BanMode(sConfigMgr->GetOption<int32>("WrongPass.BanType", uint16(BanMode::BAN_IP)));
                        int32 banTime = sConfigMgr->GetOption<int32>("WrongPass.BanTime", 600);

                        if (banType == BanMode::BAN_ACCOUNT)
                        {
                            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BNET_ACCOUNT_AUTO_BANNED);
                            stmt->SetData(0, accountId);
                        }
                        else
                        {
                            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_IP_AUTO_BANNED);
                            stmt->SetData(0, ip_address);
                        }

                        stmt->SetData(1, banTime);
                        trans->Append(stmt);

                        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_RESET_FAILED_LOGINS);
                        stmt->SetData(0, accountId);
                        trans->Append(stmt);
                    }

                    LoginDatabase.CommitTransaction(trans);
                }
            }

            JSON::Login::LoginResult loginResult;
            loginResult.set_authentication_state(JSON::Login::DONE);

            context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
            context.response.body() = ::JSON::Serialize(loginResult);
            session->SendResponse(context);
            return;
        }

        if (loginTicket.empty() || loginTicketExpiry < time(nullptr))
            loginTicket = "TC-" + ByteArrayToHexStr(Acore::Crypto::GetRandomBytes<20>());

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_AUTHENTICATION);
        stmt->SetData(0, loginTicket);
        stmt->SetData(1, uint32(time(nullptr) + _loginTicketDuration));
        stmt->SetData(2, accountId);
        callback.WithPreparedCallback([session, context = std::move(context), loginTicket = std::move(loginTicket), serverM2 = std::move(serverM2)](PreparedQueryResult) mutable
        {
            JSON::Login::LoginResult loginResult;
            loginResult.set_authentication_state(JSON::Login::DONE);
            loginResult.set_login_ticket(loginTicket);
            if (serverM2)
                loginResult.set_server_evidence_m2(*serverM2);

            context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
            context.response.body() = ::JSON::Serialize(loginResult);
            session->SendResponse(context);
        }).SetNextQuery(LoginDatabase.AsyncQuery(stmt));
    }));

    return RequestHandlerResult::Async;
}

LoginRESTService::RequestHandlerResult LoginRESTService::HandlePostLoginSrpChallenge(std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context)
{
    JSON::Login::LoginForm loginForm;
    if (!::JSON::Deserialize(context.request.body(), &loginForm))
    {
        JSON::Login::LoginResult loginResult;
        loginResult.set_authentication_state(JSON::Login::LOGIN);
        loginResult.set_error_code("UNABLE_TO_DECODE");
        loginResult.set_error_message("There was an internal error while connecting to Battle.net. Please try again later.");

        context.response.result(boost::beast::http::status::bad_request);
        context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
        context.response.body() = ::JSON::Serialize(loginResult);
        session->SendResponse(context);

        return RequestHandlerResult::Handled;
    }

    std::string login;

    for (int32 i = 0; i < loginForm.inputs_size(); ++i)
        if (loginForm.inputs(i).input_id() == "account_name")
            login = loginForm.inputs(i).value();

    Utf8ToUpperOnlyLatin(login);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_CHECK_PASSWORD_BY_EMAIL);
    stmt->SetData(0, login);

    session->QueueQuery(LoginDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([session, context = std::move(context), login = std::move(login)](PreparedQueryResult result) mutable
    {
        if (!result)
        {
            JSON::Login::LoginResult loginResult;
            loginResult.set_authentication_state(JSON::Login::DONE);
            context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
            context.response.body() = ::JSON::Serialize(loginResult);
            session->SendResponse(context);
            return;
        }

        Field* fields = result->Fetch();
        SrpVersion version = SrpVersion(fields[0].Get<int8>());
        SrpHashFunction hashFunction = SrpHashFunction::Sha256;
        std::string srpUsername = ByteArrayToHexStr(Acore::Crypto::SHA256::GetDigestOf(login));
        Acore::Crypto::SRP::Salt s = fields[1].Get<Binary, Acore::Crypto::SRP::SALT_LENGTH>();
        Acore::Crypto::SRP::Verifier v = fields[2].Get<Binary>();

        session->GetSessionState()->Srp = CreateSrpImplementation(version, hashFunction, srpUsername, s, v);
        if (!session->GetSessionState()->Srp)
        {
            context.response.result(boost::beast::http::status::internal_server_error);
            session->SendResponse(context);
            return;
        }

        JSON::Login::SrpLoginChallenge challenge;
        challenge.set_version(session->GetSessionState()->Srp->GetVersion());
        challenge.set_iterations(session->GetSessionState()->Srp->GetXIterations());
        challenge.set_modulus(session->GetSessionState()->Srp->GetN().AsHexStr());
        challenge.set_generator(session->GetSessionState()->Srp->Getg().AsHexStr());
        challenge.set_hash_function([=]
        {
            switch (hashFunction)
            {
                case SrpHashFunction::Sha256:
                    return "SHA-256";
                case SrpHashFunction::Sha512:
                    return "SHA-512";
                default:
                    break;
            }
            return "";
        }());
        challenge.set_username(srpUsername);
        challenge.set_salt(ByteArrayToHexStr(session->GetSessionState()->Srp->s));
        challenge.set_public_b(session->GetSessionState()->Srp->B.AsHexStr());

        context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
        context.response.body() = ::JSON::Serialize(challenge);
        session->SendResponse(context);
    }));

    return RequestHandlerResult::Async;
}

LoginRESTService::RequestHandlerResult LoginRESTService::HandlePostRefreshLoginTicket(std::shared_ptr<LoginHttpSession> session, HttpRequestContext& context) const
{
    std::string ticket = ExtractAuthorization(context.request);
    if (ticket.empty())
        return HandleUnauthorized(std::move(session), context);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_EXISTING_AUTHENTICATION);
    stmt->SetData(0, ticket);
    session->QueueQuery(LoginDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([this, session, context = std::move(context), ticket = std::move(ticket)](PreparedQueryResult result) mutable
    {
        JSON::Login::LoginRefreshResult loginRefreshResult;
        if (result)
        {
            uint32 loginTicketExpiry = (*result)[0].Get<uint32>();
            time_t now = time(nullptr);
            if (loginTicketExpiry > now)
            {
                loginRefreshResult.set_login_ticket_expiry(now + _loginTicketDuration);

                LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_EXISTING_AUTHENTICATION);
                stmt->SetData(0, uint32(now + _loginTicketDuration));
                stmt->SetData(1, ticket);
                LoginDatabase.Execute(stmt);
            }
            else
                loginRefreshResult.set_is_expired(true);
        }
        else
            loginRefreshResult.set_is_expired(true);

        context.response.set(boost::beast::http::field::content_type, "application/json;charset=utf-8");
        context.response.body() = ::JSON::Serialize(loginRefreshResult);
        session->SendResponse(context);
    }));

    return RequestHandlerResult::Async;
}

std::unique_ptr<Acore::Crypto::SRP::BnetSRP6Base> LoginRESTService::CreateSrpImplementation(SrpVersion version, SrpHashFunction hashFunction,
    std::string const& username, Acore::Crypto::SRP::Salt const& salt, Acore::Crypto::SRP::Verifier const& verifier)
{
    if (version == SrpVersion::v2)
    {
        if (hashFunction == SrpHashFunction::Sha256)
            return std::make_unique<Acore::Crypto::SRP::BnetSRP6v2<Acore::Crypto::SHA256>>(username, salt, verifier);
        if (hashFunction == SrpHashFunction::Sha512)
            return std::make_unique<Acore::Crypto::SRP::BnetSRP6v2<Acore::Crypto::SHA512>>(username, salt, verifier);
    }

    if (version == SrpVersion::v1)
    {
        if (hashFunction == SrpHashFunction::Sha256)
            return std::make_unique<Acore::Crypto::SRP::BnetSRP6v1<Acore::Crypto::SHA256>>(username, salt, verifier);
        if (hashFunction == SrpHashFunction::Sha512)
            return std::make_unique<Acore::Crypto::SRP::BnetSRP6v1<Acore::Crypto::SHA512>>(username, salt, verifier);
    }

    return nullptr;
}

std::shared_ptr<Acore::Net::Http::SessionState> LoginRESTService::CreateNewSessionState(boost::asio::ip::address const& address)
{
    std::shared_ptr<LoginSessionState> state = std::make_shared<LoginSessionState>();
    InitAndStoreSessionState(state, address);
    return state;
}

void LoginRESTService::MigrateLegacyPasswordHashes() const
{
    if (!LoginDatabase.Query("SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = SCHEMA() AND TABLE_NAME = 'battlenet_accounts' AND COLUMN_NAME = 'sha_pass_hash'"))
        return;

    LOG_INFO("server.http.login", "Updating password hashes...");
    uint32 const start = getMSTime();
    // the auth update query nulls salt/verifier if they cannot be converted
    // if they are non-null but s/v have been cleared, that means a legacy tool touched our auth DB (otherwise, the core might've done it itself, it used to use those hacks too)
    QueryResult result = LoginDatabase.Query("SELECT id, sha_pass_hash, IF((salt IS null) OR (verifier IS null), 0, 1) AS shouldWarn FROM battlenet_accounts WHERE sha_pass_hash != DEFAULT(sha_pass_hash) OR salt IS NULL OR verifier IS NULL");
    if (!result)
    {
        LOG_INFO("server.http.login", ">> No password hashes to update - this took us {} ms to realize", GetMSTimeDiffToNow(start));
        return;
    }

    bool hadWarning = false;
    uint32 c = 0;
    LoginDatabaseTransaction tx = LoginDatabase.BeginTransaction();
    do
    {
        uint32 const id = (*result)[0].Get<uint32>();

        Acore::Crypto::SRP::Salt salt = Acore::Crypto::GetRandomBytes<Acore::Crypto::SRP::SALT_LENGTH>();
        BigNumber x = Acore::Crypto::SHA256::GetDigestOf(salt, HexStrToByteArray<Acore::Crypto::SHA256::DIGEST_LENGTH>((*result)[1].Get<std::string>(), true));
        Acore::Crypto::SRP::Verifier verifier = Acore::Crypto::SRP::BnetSRP6v1Base::g.ModExp(x, Acore::Crypto::SRP::BnetSRP6v1Base::N).ToByteVector();

        if ((*result)[2].Get<int64>())
        {
            if (!hadWarning)
            {
                hadWarning = true;
                LOG_WARN("server.http.login",
                    "       ========\n"
                    "(!) You appear to be using an outdated external account management tool.\n"
                    "(!) Update your external tool.\n"
                    "       ========");
            }
        }

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_LOGON);
        stmt->SetData(0, AsUnderlyingType(SrpVersion::v1));
        stmt->SetData(1, salt);
        stmt->SetData(2, verifier);
        stmt->SetData(3, id);
        tx->Append(stmt);

        tx->Append(Acore::StringFormat("UPDATE battlenet_accounts SET sha_pass_hash = DEFAULT(sha_pass_hash) WHERE id = {}", id).c_str());

        if (tx->GetSize() >= 10000)
        {
            LoginDatabase.CommitTransaction(tx);
            tx = LoginDatabase.BeginTransaction();
        }

        ++c;
    } while (result->NextRow());
    LoginDatabase.CommitTransaction(tx);

    LOG_INFO("server.http.login", ">> {} password hashes updated in {} ms", c, GetMSTimeDiffToNow(start));
}
}
