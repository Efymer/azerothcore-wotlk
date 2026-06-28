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

#ifndef PerksProgramPacketsCommon_h__
#define PerksProgramPacketsCommon_h__

#include "PacketUtilities.h"

namespace WorldPackets::PerksProgram
{
// [54261] Minimal port: only PerksVendorItem is referenced by
// UF::ActivePlayerData::FrozenPerksVendorItem.
struct PerksVendorItem
{
    int32 VendorItemID = 0;
    int32 MountID = 0;
    int32 BattlePetSpeciesID = 0;
    int32 TransmogSetID = 0;
    int32 ItemModifiedAppearanceID = 0;
    int32 Field_14 = 0;
    int32 Field_18 = 0;
    int32 Price = 0;
    Timestamp<> AvailableUntil;
    bool Disabled = false;
};

ByteBuffer& operator<<(ByteBuffer& data, PerksVendorItem const& perksVendorItem);
}

#endif // PerksProgramPacketsCommon_h__
