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

#ifndef AC_DB2STRUCTURE_H
#define AC_DB2STRUCTURE_H

#include "Common.h"
#include "DB2FileLoader.h"
#include <array>

// Runtime DB2 record structures for build 3.4.3.54261. These are NOT packed: the
// DB2 file loader (AutoProduceData) reproduces fields with natural C++ alignment, so
// each struct must match TrinityCore's wotlk_classic layout byte-for-byte.
//
// NOTE: stores whose names collide with the legacy DBC set (MapEntry/sMapStore,
// AreaTableEntry, ChrRacesEntry, ...) are added here as part of the DBC->DB2 repoint
// (Task 1c.3), where the DBC version is removed in the same step. This file currently
// carries DB2-only stores that have no DBC twin.

struct LiquidMaterialEntry
{
    uint32 ID;
    int8 Flags;
    int8 LVF;
};

#endif // AC_DB2STRUCTURE_H
