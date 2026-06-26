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

#ifndef BnetRealmList_h__
#define BnetRealmList_h__

// Battlenet realm-list RPC helpers used by the bnetserver app.
//
// Rationale: AzerothCore's grunt `RealmList` (Realms/RealmList.{h,cpp}) is kept
// untouched so the authserver/worldserver path keeps working. The richer,
// proto/JSON-driven realm-list RPC that the bnetserver needs is provided here as
// a thin layer on top of the existing `sRealmList` instead of rewriting the core
// Realm model. These helpers return the generated proto types from the `proto`
// lib (RealmList.pb.h / game_utilities_service.pb.h) and serialize them with
// ProtobufJSON, matching TrinityCore's RealmList::{WriteSubRegions,
// GetRealmEntryJSON,GetRealmList,JoinRealm}.

#include "Define.h"
#include "Duration.h"
#include "Realm.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace boost::asio::ip
{
    class address;
}

namespace bgs::protocol::game_utilities::v1
{
    class ClientResponse;
    class GetAllValuesForAttributeResponse;
}

namespace Battlenet::RealmListRpc
{
    // Appends every known sub-region address (region-site-0) to the response.
    AC_SHARED_API void WriteSubRegions(bgs::protocol::game_utilities::v1::GetAllValuesForAttributeResponse* response);

    // Returns the zlib-compressed "JamJSONRealmEntry:" JSON blob for a single realm,
    // or an empty vector if the realm is unavailable/mismatched/forbidden.
    AC_SHARED_API std::vector<uint8> GetRealmEntryJSON(Battlenet::RealmHandle const& id, uint32 build, AccountTypes accountSecurityLevel);

    // Returns the zlib-compressed "JSONRealmListUpdates:" JSON blob describing all
    // realms (and removed realms) within the given sub-region.
    AC_SHARED_API std::vector<uint8> GetRealmList(uint32 build, AccountTypes accountSecurityLevel, std::string const& subRegion);

    // Validates the realm, persists the negotiated session key, and fills the
    // bnet RPC response (Param_RealmJoinTicket / Param_ServerAddresses /
    // Param_JoinSecret). Returns a BattlenetRpcErrorCode (ERROR_OK on success).
    AC_SHARED_API uint32 JoinRealm(uint32 realmAddress, uint32 build, uint32 buildVariantPlatform, uint32 buildVariantArch, uint32 buildVariantType,
        boost::asio::ip::address const& clientAddress, std::array<uint8, 32> const& clientSecret, LocaleConstant locale, std::string const& os,
        Minutes timezoneOffset, std::string const& accountName, AccountTypes accountSecurityLevel,
        bgs::protocol::game_utilities::v1::ClientResponse* response);
}

#endif // BnetRealmList_h__
