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

#ifndef AC_DB2LOADINFO_H
#define AC_DB2LOADINFO_H

#include "DB2DatabaseLoader.h"
#include "DB2Meta.h"

// Per-store metadata + field maps for build 3.4.3.54261. Layouts are generated/verified
// via WoWDBDefs LAYOUT blocks for 54261 (matching the extractor's ExtractorDB2LoadInfo.h).
//
// NOTE: the `Statement` argument is a placeholder (HotfixDatabaseStatements(0)) until the
// hotfix delivery layer (deferred item D4) registers per-table prepared statements. The
// file-load path (DB2StorageBase::Load) does not read it; only LoadFromDB() would.

struct LiquidMaterialLoadInfo
{
    // Build 3.4.3.54261 layout (WoWDBDefs LAYOUT 2CFFEA40): Flags is FT_BYTE (<8>), not FT_INT.
    static constexpr DB2MetaField MetaFields[2] =
    {
        { .Type = FT_BYTE,                 .ArraySize =  1, .IsSigned =  true },
        { .Type = FT_BYTE,                 .ArraySize =  1, .IsSigned =  true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId         = 1132538,
        .IndexField         = -1,
        .ParentIndexField   = -1,
        .FieldCount         = 2,
        .FileFieldCount     = 2,
        .LayoutHash         = 0x2CFFEA40,
        .Fields             = MetaFields
    };

    static constexpr DB2FieldMeta Fields[3] =
    {
        { false, FT_INT, "ID" },
        { true, FT_BYTE, "Flags" },
        { true, FT_BYTE, "LVF" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 3, &MetaInstance, HotfixDatabaseStatements(0) };
};

#endif // AC_DB2LOADINFO_H
