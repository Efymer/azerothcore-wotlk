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

struct SpellNameLoadInfo
{
    static constexpr DB2MetaField MetaFields[1] =
    {
        { FT_STRING, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1990283, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 1, .FileFieldCount = 1, .LayoutHash = 0xB0DD8F60, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[2] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING, "Name" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 2, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct CharacterLoadoutLoadInfo
{
    static constexpr DB2MetaField MetaFields[5] =
    {
        { FT_LONG, 1, true },
        { FT_INT, 1, false },
        { FT_BYTE, 1, true },
        { FT_INT, 1, true },
        { FT_BYTE, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1344281, .IndexField = 1, .ParentIndexField = -1,
        .FieldCount = 5, .FileFieldCount = 5, .LayoutHash = 0xCA30C801, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[5] =
    {
        { true, FT_LONG, "RaceMask" },
        { false, FT_INT, "ID" },
        { true, FT_BYTE, "ChrClassID" },
        { true, FT_INT, "Purpose" },
        { true, FT_BYTE, "ItemContext" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 5, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct CharacterLoadoutItemLoadInfo
{
    static constexpr DB2MetaField MetaFields[2] =
    {
        { FT_SHORT, 1, false },
        { FT_INT, 1, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1302846, .IndexField = -1, .ParentIndexField = 0,
        .FieldCount = 2, .FileFieldCount = 2, .LayoutHash = 0x24843CD8, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[3] =
    {
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "CharacterLoadoutID" },
        { false, FT_INT, "ItemID" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 3, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ChrCustomizationOptionLoadInfo
{
    static constexpr DB2MetaField MetaFields[12] =
    {
        { FT_STRING, 1, true },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 3384247, .IndexField = 1, .ParentIndexField = 4,
        .FieldCount = 12, .FileFieldCount = 12, .LayoutHash = 0x26DBFCD5, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[12] =
    {
        { false, FT_STRING, "Name" },
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "SecondaryID" },
        { true, FT_INT, "Flags" },
        { false, FT_INT, "ChrModelID" },                          // ParentIndexField -> must be unsigned
        { true, FT_INT, "SortIndex" },
        { true, FT_INT, "ChrCustomizationCategoryID" },
        { true, FT_INT, "OptionType" },
        { false, FT_FLOAT, "BarberShopCostModifier" },
        { true, FT_INT, "ChrCustomizationID" },
        { true, FT_INT, "ChrCustomizationReqID" },
        { true, FT_INT, "UiOrderIndex" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 12, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ChrCustomizationReqLoadInfo
{
    static constexpr DB2MetaField MetaFields[9] =
    {
        { FT_LONG, 1, true },
        { FT_STRING, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 3450453, .IndexField = 2, .ParentIndexField = -1,
        .FieldCount = 9, .FileFieldCount = 9, .LayoutHash = 0x9B25E739, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[9] =
    {
        { true, FT_LONG, "RaceMask" },
        { false, FT_STRING, "ReqSource" },
        { false, FT_INT, "ID" },
        { true, FT_INT, "Flags" },
        { true, FT_INT, "ClassMask" },
        { true, FT_INT, "AchievementID" },
        { true, FT_INT, "QuestID" },
        { true, FT_INT, "OverrideArchive" },
        { true, FT_INT, "ItemModifiedAppearanceID" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 9, &MetaInstance, HotfixDatabaseStatements(0) };
};

#endif // AC_DB2LOADINFO_H
