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

#include "MythicPlusPacketsCommon.h"
#include "ByteBuffer.h"

namespace WorldPackets
{
namespace MythicPlus
{
ByteBuffer& operator<<(ByteBuffer& data, DungeonScoreMapSummary const& dungeonScoreMapSummary)
{
    data << int32(dungeonScoreMapSummary.ChallengeModeID);
    data << float(dungeonScoreMapSummary.MapScore);
    data << int32(dungeonScoreMapSummary.BestRunLevel);
    data << int32(dungeonScoreMapSummary.BestRunDurationMS);
    data.WriteBit(dungeonScoreMapSummary.FinishedSuccess);
    data.FlushBits();

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, DungeonScoreSummary const& dungeonScoreSummary)
{
    data << float(dungeonScoreSummary.OverallScoreCurrentSeason);
    data << float(dungeonScoreSummary.LadderScoreCurrentSeason);
    data << uint32(dungeonScoreSummary.Runs.size());
    for (DungeonScoreMapSummary const& dungeonScoreMapSummary : dungeonScoreSummary.Runs)
        data << dungeonScoreMapSummary;

    return data;
}
}
}
