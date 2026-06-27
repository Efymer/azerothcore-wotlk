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

struct ItemEffectLoadInfo
{
    static constexpr DB2MetaField MetaFields[9] =
    {
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, true },
        { FT_SHORT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 969941, .IndexField = -1, .ParentIndexField = 8,
        .FieldCount = 9, .FileFieldCount = 8, .LayoutHash = 0xF2A2E644, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[10] =
    {
        { false, FT_INT, "ID" },
        { false, FT_BYTE, "LegacySlotIndex" },
        { true, FT_BYTE, "TriggerType" },
        { true, FT_SHORT, "Charges" },
        { true, FT_INT, "CoolDownMSec" },
        { true, FT_INT, "CategoryCoolDownMSec" },
        { false, FT_SHORT, "SpellCategoryID" },
        { true, FT_INT, "SpellID" },
        { false, FT_SHORT, "ChrSpecializationID" },
        { false, FT_INT, "ParentItemID" },                        // ParentIndexField -> unsigned
    };

    static constexpr DB2LoadInfo Instance{ Fields, 10, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ItemAppearanceLoadInfo
{
    static constexpr DB2MetaField MetaFields[4] =
    {
        { FT_BYTE, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 982462, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 4, .FileFieldCount = 4, .LayoutHash = 0xB7D37BC9, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[5] =
    {
        { false, FT_INT, "ID" },
        { false, FT_BYTE, "DisplayType" },
        { true, FT_INT, "ItemDisplayInfoID" },
        { true, FT_INT, "DefaultIconFileDataID" },
        { true, FT_INT, "UiOrder" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 5, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ItemModifiedAppearanceLoadInfo
{
    static constexpr DB2MetaField MetaFields[6] =
    {
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 982457, .IndexField = 0, .ParentIndexField = 1,
        .FieldCount = 6, .FileFieldCount = 6, .LayoutHash = 0xF6BAD95D, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[6] =
    {
        { false, FT_INT, "ID" },
        { false, FT_INT, "ItemID" },                              // ParentIndexField -> unsigned
        { true, FT_INT, "ItemAppearanceModifierID" },
        { true, FT_INT, "ItemAppearanceID" },
        { true, FT_INT, "OrderIndex" },
        { true, FT_INT, "TransmogSourceTypeEnum" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 6, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct PowerTypeLoadInfo
{
    static constexpr DB2MetaField MetaFields[12] =
    {
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_BYTE, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_SHORT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1266022, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 12, .FileFieldCount = 12, .LayoutHash = 0xA1F55F15, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[13] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING_NOT_LOCALIZED, "NameGlobalStringTag" },
        { false, FT_STRING_NOT_LOCALIZED, "CostGlobalStringTag" },
        { true, FT_BYTE, "PowerTypeEnum" },
        { true, FT_INT, "MinPower" },
        { true, FT_INT, "MaxBasePower" },
        { true, FT_INT, "CenterPower" },
        { true, FT_INT, "DefaultPower" },
        { true, FT_INT, "DisplayModifier" },
        { true, FT_INT, "RegenInterruptTimeMS" },
        { false, FT_FLOAT, "RegenPeace" },
        { false, FT_FLOAT, "RegenCombat" },
        { true, FT_SHORT, "Flags" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 13, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ItemSparseLoadInfo
{
    static constexpr DB2MetaField MetaFields[73] =
    {
        { FT_LONG, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_FLOAT, 1, true },
        { FT_INT, 1, false },
        { FT_FLOAT, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_FLOAT, 10, true },
        { FT_INT, 10, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_FLOAT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_INT, 4, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 2, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 5, false },
        { FT_SHORT, 5, false },
        { FT_SHORT, 7, true },
        { FT_SHORT, 1, false },
        { FT_SHORT, 10, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 3, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 10, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1572924, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 73, .FileFieldCount = 73, .LayoutHash = 0xD532973D, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[130] =
    {
        { false, FT_INT, "ID" },
        { true, FT_LONG, "AllowableRace" },
        { false, FT_STRING, "Description" },
        { false, FT_STRING, "Display3" },
        { false, FT_STRING, "Display2" },
        { false, FT_STRING, "Display1" },
        { false, FT_STRING, "Display" },
        { false, FT_FLOAT, "DmgVariance" },
        { false, FT_INT, "DurationInInventory" },
        { false, FT_FLOAT, "QualityModifier" },
        { false, FT_INT, "BagFamily" },
        { true, FT_INT, "StartQuestID" },
        { false, FT_FLOAT, "ItemRange" },
        { false, FT_FLOAT, "StatPercentageOfSocket1" },
        { false, FT_FLOAT, "StatPercentageOfSocket2" },
        { false, FT_FLOAT, "StatPercentageOfSocket3" },
        { false, FT_FLOAT, "StatPercentageOfSocket4" },
        { false, FT_FLOAT, "StatPercentageOfSocket5" },
        { false, FT_FLOAT, "StatPercentageOfSocket6" },
        { false, FT_FLOAT, "StatPercentageOfSocket7" },
        { false, FT_FLOAT, "StatPercentageOfSocket8" },
        { false, FT_FLOAT, "StatPercentageOfSocket9" },
        { false, FT_FLOAT, "StatPercentageOfSocket10" },
        { true, FT_INT, "StatPercentEditor1" },
        { true, FT_INT, "StatPercentEditor2" },
        { true, FT_INT, "StatPercentEditor3" },
        { true, FT_INT, "StatPercentEditor4" },
        { true, FT_INT, "StatPercentEditor5" },
        { true, FT_INT, "StatPercentEditor6" },
        { true, FT_INT, "StatPercentEditor7" },
        { true, FT_INT, "StatPercentEditor8" },
        { true, FT_INT, "StatPercentEditor9" },
        { true, FT_INT, "StatPercentEditor10" },
        { true, FT_INT, "Stackable" },
        { true, FT_INT, "MaxCount" },
        { true, FT_INT, "MinReputation" },
        { false, FT_INT, "RequiredAbility" },
        { false, FT_INT, "SellPrice" },
        { false, FT_INT, "BuyPrice" },
        { false, FT_INT, "VendorStackCount" },
        { false, FT_FLOAT, "PriceVariance" },
        { false, FT_FLOAT, "PriceRandomValue" },
        { true, FT_INT, "Flags1" },
        { true, FT_INT, "Flags2" },
        { true, FT_INT, "Flags3" },
        { true, FT_INT, "Flags4" },
        { true, FT_INT, "FactionRelated" },
        { true, FT_INT, "ModifiedCraftingReagentItemID" },
        { true, FT_INT, "ContentTuningID" },
        { true, FT_INT, "PlayerLevelToItemLevelCurveID" },
        { false, FT_INT, "MaxDurability" },
        { false, FT_SHORT, "ItemNameDescriptionID" },
        { false, FT_SHORT, "RequiredTransmogHoliday" },
        { false, FT_SHORT, "RequiredHoliday" },
        { false, FT_SHORT, "LimitCategory" },
        { false, FT_SHORT, "GemProperties" },
        { false, FT_SHORT, "SocketMatchEnchantmentId" },
        { false, FT_SHORT, "TotemCategoryID" },
        { false, FT_SHORT, "InstanceBound" },
        { false, FT_SHORT, "ZoneBound1" },
        { false, FT_SHORT, "ZoneBound2" },
        { false, FT_SHORT, "ItemSet" },
        { false, FT_SHORT, "LockID" },
        { false, FT_SHORT, "PageID" },
        { false, FT_SHORT, "ItemDelay" },
        { false, FT_SHORT, "MinFactionID" },
        { false, FT_SHORT, "RequiredSkillRank" },
        { false, FT_SHORT, "RequiredSkill" },
        { false, FT_SHORT, "ItemLevel" },
        { true, FT_SHORT, "AllowableClass" },
        { false, FT_SHORT, "ItemRandomSuffixGroupID" },
        { false, FT_SHORT, "RandomSelect" },
        { false, FT_SHORT, "MinDamage1" },
        { false, FT_SHORT, "MinDamage2" },
        { false, FT_SHORT, "MinDamage3" },
        { false, FT_SHORT, "MinDamage4" },
        { false, FT_SHORT, "MinDamage5" },
        { false, FT_SHORT, "MaxDamage1" },
        { false, FT_SHORT, "MaxDamage2" },
        { false, FT_SHORT, "MaxDamage3" },
        { false, FT_SHORT, "MaxDamage4" },
        { false, FT_SHORT, "MaxDamage5" },
        { true, FT_SHORT, "Resistances1" },
        { true, FT_SHORT, "Resistances2" },
        { true, FT_SHORT, "Resistances3" },
        { true, FT_SHORT, "Resistances4" },
        { true, FT_SHORT, "Resistances5" },
        { true, FT_SHORT, "Resistances6" },
        { true, FT_SHORT, "Resistances7" },
        { false, FT_SHORT, "ScalingStatDistributionID" },
        { true, FT_SHORT, "StatModifierBonusAmount1" },
        { true, FT_SHORT, "StatModifierBonusAmount2" },
        { true, FT_SHORT, "StatModifierBonusAmount3" },
        { true, FT_SHORT, "StatModifierBonusAmount4" },
        { true, FT_SHORT, "StatModifierBonusAmount5" },
        { true, FT_SHORT, "StatModifierBonusAmount6" },
        { true, FT_SHORT, "StatModifierBonusAmount7" },
        { true, FT_SHORT, "StatModifierBonusAmount8" },
        { true, FT_SHORT, "StatModifierBonusAmount9" },
        { true, FT_SHORT, "StatModifierBonusAmount10" },
        { false, FT_BYTE, "ExpansionID" },
        { false, FT_BYTE, "ArtifactID" },
        { false, FT_BYTE, "SpellWeight" },
        { false, FT_BYTE, "SpellWeightCategory" },
        { false, FT_BYTE, "SocketType1" },
        { false, FT_BYTE, "SocketType2" },
        { false, FT_BYTE, "SocketType3" },
        { false, FT_BYTE, "SheatheType" },
        { false, FT_BYTE, "Material" },
        { false, FT_BYTE, "PageMaterialID" },
        { false, FT_BYTE, "LanguageID" },
        { false, FT_BYTE, "Bonding" },
        { false, FT_BYTE, "DamageDamageType" },
        { true, FT_BYTE, "StatModifierBonusStat1" },
        { true, FT_BYTE, "StatModifierBonusStat2" },
        { true, FT_BYTE, "StatModifierBonusStat3" },
        { true, FT_BYTE, "StatModifierBonusStat4" },
        { true, FT_BYTE, "StatModifierBonusStat5" },
        { true, FT_BYTE, "StatModifierBonusStat6" },
        { true, FT_BYTE, "StatModifierBonusStat7" },
        { true, FT_BYTE, "StatModifierBonusStat8" },
        { true, FT_BYTE, "StatModifierBonusStat9" },
        { true, FT_BYTE, "StatModifierBonusStat10" },
        { false, FT_BYTE, "ContainerSlots" },
        { false, FT_BYTE, "RequiredPVPMedal" },
        { false, FT_BYTE, "RequiredPVPRank" },
        { true, FT_BYTE, "InventoryType" },
        { true, FT_BYTE, "OverallQualityID" },
        { false, FT_BYTE, "AmmunitionType" },
        { true, FT_BYTE, "RequiredLevel" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 130, &MetaInstance, HotfixDatabaseStatements(0) };
};

#endif // AC_DB2LOADINFO_H
