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

#include "DatabaseEnv.h"
#include "Log.h"
#include "QueryResult.h"
#include "Timer.h"
#include "WorldGlobals.h"

WorldGlobals* WorldGlobals::instance()
{
    static WorldGlobals instance;
    return &instance;
}

void WorldGlobals::LoadAntiDosOpcodePolicies()
{
    uint32 oldMSTime = getMSTime();

    _antiDosOpcodePolicies = {};

    QueryResult result = WorldDatabase.Query("SELECT Opcode, Policy, MaxAllowedCount FROM antidos_opcode_policies");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 AntiDos Opcode Policies. DB table `antidos_opcode_policies` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        // brick B: opcodes are uint32 (OpcodeClient) for the 3.4.3 wire protocol; index by the sparse CMSG array slot
        uint32 opcode = fields[0].Get<uint32>();
        std::ptrdiff_t index = GetOpcodeArrayIndex(static_cast<OpcodeClient>(opcode));
        if (index < 0 || index >= std::ptrdiff_t(NUM_CMSG_OPCODES))
        {
            LOG_ERROR("server.loading", "Unkown opcode {} in table `antidos_opcode_policies`, skipping.", opcode);
            continue;
        }

        std::unique_ptr<AntiDosOpcodePolicy> policy = std::make_unique<AntiDosOpcodePolicy>();
        policy->Policy = fields[1].Get<uint8>();
        policy->MaxAllowedCount = fields[2].Get<uint16>();

        _antiDosOpcodePolicies[index] = std::move(policy);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} AntiDos Opcode Policies in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

AntiDosOpcodePolicy const* WorldGlobals::GetAntiDosPolicyForOpcode(uint32 opcode)
{
    std::ptrdiff_t index = GetOpcodeArrayIndex(static_cast<OpcodeClient>(opcode));
    if (index < 0 || index >= std::ptrdiff_t(NUM_CMSG_OPCODES))
        return nullptr;

    return _antiDosOpcodePolicies[index].get();
}
