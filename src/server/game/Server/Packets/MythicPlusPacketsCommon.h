/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MythicPlusPacketsCommon_h__
#define MythicPlusPacketsCommon_h__

#include "Define.h"
#include <vector>

class ByteBuffer;

namespace WorldPackets
{
    namespace MythicPlus
    {
        // [54261] Minimal port: only DungeonScoreSummary (+ nested DungeonScoreMapSummary)
        // is referenced by UF::PlayerData::DungeonScore. The full Mythic+ score tree
        // (DungeonScoreData / seasons / runs / members) is intentionally not ported.
        struct DungeonScoreMapSummary
        {
            int32 ChallengeModeID = 0;
            float MapScore = 0.0f;
            int32 BestRunLevel = 0;
            int32 BestRunDurationMS = 0;
            bool FinishedSuccess = false;
        };

        struct DungeonScoreSummary
        {
            float OverallScoreCurrentSeason = 0.0f;
            float LadderScoreCurrentSeason = 0.0f;
            std::vector<DungeonScoreMapSummary> Runs;
        };

        ByteBuffer& operator<<(ByteBuffer& data, DungeonScoreMapSummary const& dungeonScoreMapSummary);
        ByteBuffer& operator<<(ByteBuffer& data, DungeonScoreSummary const& dungeonScoreSummary);
    }
}

#endif // MythicPlusPacketsCommon_h__
