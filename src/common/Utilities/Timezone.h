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

#ifndef ACORE_TIMEZONE_H
#define ACORE_TIMEZONE_H

// Minimal STUB of TrinityCore's Timezone helpers.
//
// The bnetserver references Timezone::GetOffsetByHash to translate a client
// timezone hash into a UTC offset. AzerothCore has no timezone database, so this
// returns 0 (UTC) for every hash. Replace with a real lookup table if accurate
// per-client timezone offsets are ever required.

#include "Define.h"
#include "Duration.h"

namespace Acore::Timezone
{
    // STUB: always returns 0 minutes (UTC). hash is ignored.
    inline Minutes GetOffsetByHash(uint32 /*hash*/)
    {
        return Minutes(0);
    }
}

#endif // ACORE_TIMEZONE_H
