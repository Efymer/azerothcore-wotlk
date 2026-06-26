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

#include "BnetRealmList.h"
#include "BattlenetRpcErrorCodes.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ProtobufJSON.h"
#include "RealmList.h"
#include "game_utilities_service.pb.h"
#include "RealmList.pb.h"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <zlib.h>

namespace
{
// Population states matching the bnet RealmPopulationState client enum.
enum class BnetPopulationState : uint32
{
    Offline     = 0,
    Low         = 1,
    Medium      = 2,
    High        = 3,
    New         = 4,
    Recommended = 5,
    Full        = 6,
    Locked      = 7
};

// AzerothCore stores realm population as a float and uses the legacy RealmFlags
// bitfield. Convert that to the client-facing population state, mirroring TC's
// ConvertLegacyPopulationState.
BnetPopulationState GetPopulationState(RealmFlags flags, float population)
{
    if (flags & REALM_FLAG_OFFLINE)
        return BnetPopulationState::Offline;
    if (flags & REALM_FLAG_RECOMMENDED)
        return BnetPopulationState::Recommended;
    if (flags & REALM_FLAG_NEW)
        return BnetPopulationState::New;
    if ((flags & REALM_FLAG_FULL) || population > 0.95f)
        return BnetPopulationState::Full;
    if (population > 0.66f)
        return BnetPopulationState::High;
    if (population > 0.33f)
        return BnetPopulationState::Medium;
    return BnetPopulationState::Low;
}

// config id by realm type, used for cfgConfigsID (mirrors TC's Realm::ConfigIdByType)
uint32 const ConfigIdByType[MAX_CLIENT_REALM_TYPE] =
{
    0, 6, 0, 0, 1, 0, 2, 0, 3, 0, 0, 0, 0, 0
};

uint32 GetConfigId(uint8 type)
{
    return type < MAX_CLIENT_REALM_TYPE ? ConfigIdByType[type] : ConfigIdByType[0];
}

// AzerothCore's grunt RealmList stores a plain uint32 realm id; the bnet layer
// needs a Region/Site/Realm handle. Region/Site are not modelled in the
// `realmlist` table, so they default to 0 here (single development sub-region).
// NOTE/STUB: extend the `realmlist` schema + LOGIN_SEL_REALMLIST with region and
// battlegroup columns to support multiple bnet sub-regions.
Battlenet::RealmHandle ToBnetHandle(::RealmHandle const& id)
{
    return Battlenet::RealmHandle(uint8(0), uint8(0), id.Realm);
}

bool CompressJson(std::string const& json, std::vector<uint8>* compressed)
{
    uLong uncompressedLength = uLong(json.length() + 1);
    uLong compressedLength = compressBound(uLong(json.length()));
    compressed->resize(compressedLength + 4);
    std::memcpy(compressed->data(), &uncompressedLength, sizeof(uncompressedLength));

    if (compress(compressed->data() + 4, &compressedLength, reinterpret_cast<uint8 const*>(json.data()), uncompressedLength) != Z_OK)
    {
        compressed->clear();
        return false;
    }

    compressed->resize(compressedLength + 4);   // trim excess bytes
    return true;
}

void FillRealmEntry(Realm const& realm, uint32 clientBuild, AccountTypes accountSecurityLevel, JSON::RealmList::RealmEntry* realmEntry)
{
    Battlenet::RealmHandle id = ToBnetHandle(realm.Id);
    BnetPopulationState population = GetPopulationState(realm.Flags, realm.PopulationLevel);

    realmEntry->set_wowrealmaddress(id.GetAddress());
    realmEntry->set_cfgtimezonesid(1);
    if (accountSecurityLevel >= realm.AllowedSecurityLevel || population == BnetPopulationState::Offline)
        realmEntry->set_populationstate(uint32(population));
    else
        realmEntry->set_populationstate(uint32(BnetPopulationState::Locked));

    realmEntry->set_cfgcategoriesid(realm.Timezone);

    JSON::RealmList::ClientVersion* version = realmEntry->mutable_version();
    if (RealmBuildInfo const* buildInfo = sRealmList->GetBuildInfo(realm.Build))
    {
        version->set_versionmajor(buildInfo->MajorVersion);
        version->set_versionminor(buildInfo->MinorVersion);
        version->set_versionrevision(buildInfo->BugfixVersion);
        version->set_versionbuild(buildInfo->Build);
    }
    else
    {
        version->set_versionmajor(6);
        version->set_versionminor(2);
        version->set_versionrevision(4);
        version->set_versionbuild(realm.Build);
    }

    uint32 flags = realm.Flags;
    if (realm.Build != clientBuild)
        flags |= REALM_FLAG_VERSION_MISMATCH;

    realmEntry->set_cfgrealmsid(id.Realm);
    realmEntry->set_flags(flags);
    realmEntry->set_name(realm.Name);
    realmEntry->set_cfgconfigsid(GetConfigId(realm.Type));
    realmEntry->set_cfglanguagesid(1);
}
}

namespace Battlenet::RealmListRpc
{
void WriteSubRegions(bgs::protocol::game_utilities::v1::GetAllValuesForAttributeResponse* response)
{
    std::set<std::string> subRegions;
    for (auto const& [handle, realm] : sRealmList->GetRealms())
        subRegions.insert(ToBnetHandle(handle).GetSubRegionAddress());

    for (std::string const& subRegion : subRegions)
        response->add_attribute_value()->set_string_value(subRegion);
}

std::vector<uint8> GetRealmEntryJSON(Battlenet::RealmHandle const& id, uint32 build, AccountTypes accountSecurityLevel)
{
    std::vector<uint8> compressed;
    if (Realm const* realm = sRealmList->GetRealm(::RealmHandle(id.Realm)))
    {
        BnetPopulationState population = GetPopulationState(realm->Flags, realm->PopulationLevel);
        if (population != BnetPopulationState::Offline && realm->Build == build && accountSecurityLevel >= realm->AllowedSecurityLevel)
        {
            JSON::RealmList::RealmEntry realmEntry;
            FillRealmEntry(*realm, build, accountSecurityLevel, &realmEntry);

            std::string json = "JamJSONRealmEntry:" + JSON::Serialize(realmEntry);
            CompressJson(json, &compressed);
        }
    }

    return compressed;
}

std::vector<uint8> GetRealmList(uint32 build, AccountTypes accountSecurityLevel, std::string const& subRegion)
{
    JSON::RealmList::RealmListUpdates realmList;
    for (auto const& [handle, realm] : sRealmList->GetRealms())
    {
        if (ToBnetHandle(handle).GetSubRegionAddress() != subRegion)
            continue;

        JSON::RealmList::RealmListUpdatePart* state = realmList.add_updates();
        FillRealmEntry(realm, build, accountSecurityLevel, state->mutable_update());
        state->set_deleting(false);
    }

    // NOTE: AzerothCore's grunt RealmList does not track removed realms between
    // updates, so no "deleting" entries are emitted here.

    std::string json = "JSONRealmListUpdates:" + JSON::Serialize(realmList);
    std::vector<uint8> compressed;
    CompressJson(json, &compressed);
    return compressed;
}

uint32 JoinRealm(uint32 realmAddress, uint32 build, uint32 buildVariantPlatform, uint32 buildVariantArch, uint32 buildVariantType,
    boost::asio::ip::address const& clientAddress, std::array<uint8, 32> const& clientSecret, LocaleConstant locale, std::string const& os,
    Minutes timezoneOffset, std::string const& accountName, AccountTypes accountSecurityLevel,
    bgs::protocol::game_utilities::v1::ClientResponse* response)
{
    Battlenet::RealmHandle id(realmAddress);
    Realm const* realm = sRealmList->GetRealm(::RealmHandle(id.Realm));
    if (!realm)
        return ERROR_UTIL_SERVER_UNKNOWN_REALM;

    BnetPopulationState population = GetPopulationState(realm->Flags, realm->PopulationLevel);
    if (population == BnetPopulationState::Offline || realm->Build != build || accountSecurityLevel < realm->AllowedSecurityLevel)
        return ERROR_USER_SERVER_NOT_PERMITTED_ON_REALM;

    boost::asio::ip::tcp::endpoint endpoint = realm->GetAddressForClient(clientAddress);
    boost::asio::ip::address addressForClient = endpoint.address();

    JSON::RealmList::RealmListServerIPAddresses serverAddresses;
    JSON::RealmList::RealmIPAddressFamily* addressFamily = serverAddresses.add_families();
    addressFamily->set_family(addressForClient.is_v6() ? 2 : 1);

    JSON::RealmList::IPAddress* address = addressFamily->add_addresses();
    address->set_ip(addressForClient.to_string());
    address->set_port(realm->Port);

    std::string json = "JSONRealmListServerIPAddresses:" + JSON::Serialize(serverAddresses);
    std::vector<uint8> compressed;
    if (!CompressJson(json, &compressed))
        return ERROR_UTIL_SERVER_FAILED_TO_SERIALIZE_RESPONSE;

    std::array<uint8, 32> serverSecret = Acore::Crypto::GetRandomBytes<32>();

    std::array<uint8, 64> keyData;
    auto keyDestItr = std::copy(clientSecret.begin(), clientSecret.end(), keyData.begin());
    std::copy(serverSecret.begin(), serverSecret.end(), keyDestItr);

    // STUB/LIMITATION: AzerothCore's `account.session_key` is binary(40); only the
    // first 40 bytes of the 64-byte negotiated key are persisted here. A wider
    // column (or a dedicated bnet session table) is required before the worldserver
    // can validate the full key. Build/timezone offset are not persisted either.
    // timezoneOffset is accepted for API parity with the bnet client request but is
    // not persisted yet (no column in `account`); silence the unused warning.
    (void)timezoneOffset;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LOGIN_INFO);
    std::vector<uint8> sessionKey(keyData.begin(), keyData.begin() + 40);
    stmt->SetData(0, sessionKey);
    stmt->SetData(1, clientAddress.to_string());
    stmt->SetData(2, uint8(locale));
    stmt->SetData(3, os);
    stmt->SetData(4, accountName);
    LoginDatabase.DirectExecute(stmt);

    JSON::RealmList::RealmJoinTicket joinTicket;
    joinTicket.set_gameaccount(accountName);
    joinTicket.set_platform(buildVariantPlatform);
    joinTicket.set_clientarch(buildVariantArch);
    joinTicket.set_type(buildVariantType);

    bgs::protocol::Attribute* attribute = response->add_attribute();
    attribute->set_name("Param_RealmJoinTicket");
    attribute->mutable_value()->set_blob_value(JSON::Serialize(joinTicket));

    attribute = response->add_attribute();
    attribute->set_name("Param_ServerAddresses");
    attribute->mutable_value()->set_blob_value(compressed.data(), compressed.size());

    attribute = response->add_attribute();
    attribute->set_name("Param_JoinSecret");
    attribute->mutable_value()->set_blob_value(serverSecret.data(), serverSecret.size());
    return ERROR_OK;
}
}
