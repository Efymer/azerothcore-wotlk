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

#ifndef ItemPacketsCommon_h__
#define ItemPacketsCommon_h__

#include "Define.h"
#include <vector>

class ByteBuffer;

namespace WorldPackets
{
    namespace Item
    {
        // [54261] Minimal port: only ItemBonusKey is referenced by UF::ItemData.
        // The full TrinityCore ItemPacketsCommon (ItemInstance, ItemModList, ItemGemData, ...)
        // is intentionally not ported (AC has no Azerite/structured-item packet path yet).
        struct ItemBonusKey
        {
            int32 ItemID = 0;
            std::vector<int32> BonusListIDs;

            bool operator==(ItemBonusKey const& right) const;
        };

        ByteBuffer& operator<<(ByteBuffer& data, ItemBonusKey const& itemBonusKey);
    }
}

#endif // ItemPacketsCommon_h__
