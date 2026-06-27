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

#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "AuthenticationPackets.h"
#include "GameTime.h"
#include "Realm.h"

// 3.4.3: the modern SMSG_AUTH_RESPONSE (WorldPackets::Auth::AuthResponse). The legacy 3.3.5a byte
// layout (uint8 code + billing + expansion + queue) is replaced. The success block's race/class
// availability + character templates stay empty (brick-C stubs) until the world-entry DB2 store
// layer lands; an empty list still produces a valid packet the client parses (shows no templates).
void WorldSession::SendAuthResponse(uint8 code, bool shortForm, uint32 queuePos)
{
    WorldPackets::Auth::AuthResponse response;
    // 3.4.3 Result uses BattlenetRpcErrorCodes: success and "queued" are ERROR_OK (0), with the queue
    // signalled by WaitInfo. AC's legacy ResponseCodes (AUTH_OK = 12, …) would read as an error to the
    // modern client, so map the accept/queue path to 0; other codes are treated as errors as-is.
    response.Result = (code == AUTH_OK || code == AUTH_WAIT_QUEUE) ? 0u /*ERROR_OK*/ : uint32(code);

    if (code == AUTH_OK)
    {
        response.SuccessInfo.emplace();
        response.SuccessInfo->ActiveExpansionLevel = Expansion();
        response.SuccessInfo->AccountExpansionLevel = Expansion();
        // AzerothCore's grunt RealmHandle only carries the realm index (Region/Site default to 0),
        // so the virtual-realm address is just the realm id.
        response.SuccessInfo->VirtualRealmAddress = realm.Id.Realm;
        response.SuccessInfo->Time = int32(GameTime::GetGameTime().count());
        // Advertise this realm as the (local, non-internal) home virtual realm.
        response.SuccessInfo->VirtualRealms.emplace_back(realm.Id.Realm, true, false, realm.Name, realm.Name);
    }
    else if (code == AUTH_WAIT_QUEUE)
    {
        response.WaitInfo.emplace();
        response.WaitInfo->WaitCount = queuePos;
    }

    SendPacket(response.Write());
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPacket data(SMSG_CACHE_VERSION, 4);
    data << uint32(version);
    SendPacket(&data);
}
