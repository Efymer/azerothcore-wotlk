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

struct PlayerConditionLoadInfo
{
    static constexpr DB2MetaField MetaFields[81] =
    {
        { FT_LONG, 1, true },
        { FT_STRING, 1, true },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, false },
        { FT_SHORT, 1, false },
        { FT_INT, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_INT, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_SHORT, 4, false },
        { FT_SHORT, 4, false },
        { FT_SHORT, 4, false },
        { FT_INT, 3, false },
        { FT_BYTE, 3, false },
        { FT_INT, 4, false },
        { FT_INT, 4, false },
        { FT_INT, 4, false },
        { FT_INT, 4, true },
        { FT_INT, 4, true },
        { FT_INT, 4, false },
        { FT_SHORT, 2, false },
        { FT_INT, 2, false },
        { FT_INT, 4, true },
        { FT_BYTE, 4, false },
        { FT_SHORT, 4, false },
        { FT_SHORT, 4, false },
        { FT_BYTE, 4, false },
        { FT_BYTE, 4, false },
        { FT_INT, 4, false },
        { FT_INT, 4, false },
        { FT_INT, 4, false },
        { FT_INT, 6, false },
        { FT_INT, 2, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1045411, .IndexField = 2, .ParentIndexField = -1,
        .FieldCount = 81, .FileFieldCount = 81, .LayoutHash = 0xEFCD230E, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[147] =
    {
        { true, FT_LONG, "RaceMask" },
        { false, FT_STRING, "FailureDescription" },
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "MinLevel" },
        { false, FT_SHORT, "MaxLevel" },
        { true, FT_INT, "ClassMask" },
        { false, FT_INT, "SkillLogic" },
        { false, FT_BYTE, "LanguageID" },
        { false, FT_BYTE, "MinLanguage" },
        { true, FT_INT, "MaxLanguage" },
        { false, FT_SHORT, "MaxFactionID" },
        { false, FT_BYTE, "MaxReputation" },
        { false, FT_INT, "ReputationLogic" },
        { true, FT_BYTE, "CurrentPvpFaction" },
        { false, FT_BYTE, "PvpMedal" },
        { false, FT_INT, "PrevQuestLogic" },
        { false, FT_INT, "CurrQuestLogic" },
        { false, FT_INT, "CurrentCompletedQuestLogic" },
        { false, FT_INT, "SpellLogic" },
        { false, FT_INT, "ItemLogic" },
        { false, FT_BYTE, "ItemFlags" },
        { false, FT_INT, "AuraSpellLogic" },
        { false, FT_SHORT, "WorldStateExpressionID" },
        { false, FT_BYTE, "WeatherID" },
        { false, FT_BYTE, "PartyStatus" },
        { false, FT_BYTE, "LifetimeMaxPVPRank" },
        { false, FT_INT, "AchievementLogic" },
        { true, FT_BYTE, "Gender" },
        { true, FT_BYTE, "NativeGender" },
        { false, FT_INT, "AreaLogic" },
        { false, FT_INT, "LfgLogic" },
        { false, FT_INT, "CurrencyLogic" },
        { false, FT_INT, "QuestKillID" },
        { false, FT_INT, "QuestKillLogic" },
        { true, FT_BYTE, "MinExpansionLevel" },
        { true, FT_BYTE, "MaxExpansionLevel" },
        { true, FT_INT, "MinAvgItemLevel" },
        { true, FT_INT, "MaxAvgItemLevel" },
        { false, FT_SHORT, "MinAvgEquippedItemLevel" },
        { false, FT_SHORT, "MaxAvgEquippedItemLevel" },
        { false, FT_BYTE, "PhaseUseFlags" },
        { false, FT_SHORT, "PhaseID" },
        { false, FT_INT, "PhaseGroupID" },
        { false, FT_BYTE, "Flags" },
        { true, FT_BYTE, "ChrSpecializationIndex" },
        { true, FT_BYTE, "ChrSpecializationRole" },
        { false, FT_INT, "ModifierTreeID" },
        { true, FT_BYTE, "PowerType" },
        { false, FT_BYTE, "PowerTypeComp" },
        { false, FT_BYTE, "PowerTypeValue" },
        { true, FT_INT, "WeaponSubclassMask" },
        { false, FT_BYTE, "MaxGuildLevel" },
        { false, FT_BYTE, "MinGuildLevel" },
        { true, FT_BYTE, "MaxExpansionTier" },
        { true, FT_BYTE, "MinExpansionTier" },
        { false, FT_BYTE, "MinPVPRank" },
        { false, FT_BYTE, "MaxPVPRank" },
        { false, FT_SHORT, "SkillID1" },
        { false, FT_SHORT, "SkillID2" },
        { false, FT_SHORT, "SkillID3" },
        { false, FT_SHORT, "SkillID4" },
        { false, FT_SHORT, "MinSkill1" },
        { false, FT_SHORT, "MinSkill2" },
        { false, FT_SHORT, "MinSkill3" },
        { false, FT_SHORT, "MinSkill4" },
        { false, FT_SHORT, "MaxSkill1" },
        { false, FT_SHORT, "MaxSkill2" },
        { false, FT_SHORT, "MaxSkill3" },
        { false, FT_SHORT, "MaxSkill4" },
        { false, FT_INT, "MinFactionID1" },
        { false, FT_INT, "MinFactionID2" },
        { false, FT_INT, "MinFactionID3" },
        { false, FT_BYTE, "MinReputation1" },
        { false, FT_BYTE, "MinReputation2" },
        { false, FT_BYTE, "MinReputation3" },
        { false, FT_INT, "PrevQuestID1" },
        { false, FT_INT, "PrevQuestID2" },
        { false, FT_INT, "PrevQuestID3" },
        { false, FT_INT, "PrevQuestID4" },
        { false, FT_INT, "CurrQuestID1" },
        { false, FT_INT, "CurrQuestID2" },
        { false, FT_INT, "CurrQuestID3" },
        { false, FT_INT, "CurrQuestID4" },
        { false, FT_INT, "CurrentCompletedQuestID1" },
        { false, FT_INT, "CurrentCompletedQuestID2" },
        { false, FT_INT, "CurrentCompletedQuestID3" },
        { false, FT_INT, "CurrentCompletedQuestID4" },
        { true, FT_INT, "SpellID1" },
        { true, FT_INT, "SpellID2" },
        { true, FT_INT, "SpellID3" },
        { true, FT_INT, "SpellID4" },
        { true, FT_INT, "ItemID1" },
        { true, FT_INT, "ItemID2" },
        { true, FT_INT, "ItemID3" },
        { true, FT_INT, "ItemID4" },
        { false, FT_INT, "ItemCount1" },
        { false, FT_INT, "ItemCount2" },
        { false, FT_INT, "ItemCount3" },
        { false, FT_INT, "ItemCount4" },
        { false, FT_SHORT, "Explored1" },
        { false, FT_SHORT, "Explored2" },
        { false, FT_INT, "Time1" },
        { false, FT_INT, "Time2" },
        { true, FT_INT, "AuraSpellID1" },
        { true, FT_INT, "AuraSpellID2" },
        { true, FT_INT, "AuraSpellID3" },
        { true, FT_INT, "AuraSpellID4" },
        { false, FT_BYTE, "AuraStacks1" },
        { false, FT_BYTE, "AuraStacks2" },
        { false, FT_BYTE, "AuraStacks3" },
        { false, FT_BYTE, "AuraStacks4" },
        { false, FT_SHORT, "Achievement1" },
        { false, FT_SHORT, "Achievement2" },
        { false, FT_SHORT, "Achievement3" },
        { false, FT_SHORT, "Achievement4" },
        { false, FT_SHORT, "AreaID1" },
        { false, FT_SHORT, "AreaID2" },
        { false, FT_SHORT, "AreaID3" },
        { false, FT_SHORT, "AreaID4" },
        { false, FT_BYTE, "LfgStatus1" },
        { false, FT_BYTE, "LfgStatus2" },
        { false, FT_BYTE, "LfgStatus3" },
        { false, FT_BYTE, "LfgStatus4" },
        { false, FT_BYTE, "LfgCompare1" },
        { false, FT_BYTE, "LfgCompare2" },
        { false, FT_BYTE, "LfgCompare3" },
        { false, FT_BYTE, "LfgCompare4" },
        { false, FT_INT, "LfgValue1" },
        { false, FT_INT, "LfgValue2" },
        { false, FT_INT, "LfgValue3" },
        { false, FT_INT, "LfgValue4" },
        { false, FT_INT, "CurrencyID1" },
        { false, FT_INT, "CurrencyID2" },
        { false, FT_INT, "CurrencyID3" },
        { false, FT_INT, "CurrencyID4" },
        { false, FT_INT, "CurrencyCount1" },
        { false, FT_INT, "CurrencyCount2" },
        { false, FT_INT, "CurrencyCount3" },
        { false, FT_INT, "CurrencyCount4" },
        { false, FT_INT, "QuestKillMonster1" },
        { false, FT_INT, "QuestKillMonster2" },
        { false, FT_INT, "QuestKillMonster3" },
        { false, FT_INT, "QuestKillMonster4" },
        { false, FT_INT, "QuestKillMonster5" },
        { false, FT_INT, "QuestKillMonster6" },
        { true, FT_INT, "MovementFlags1" },
        { true, FT_INT, "MovementFlags2" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 147, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ChrRacesLoadInfo
{
    static constexpr DB2MetaField MetaFields[57] =
    {
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 3, false },
        { FT_INT, 3, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_FLOAT, 3, true },
        { FT_FLOAT, 1, true },
        { FT_FLOAT, 3, true },
        { FT_FLOAT, 3, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1305311, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 57, .FileFieldCount = 57, .LayoutHash = 0x756C30D6, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[68] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING_NOT_LOCALIZED, "ClientPrefix" },
        { false, FT_STRING_NOT_LOCALIZED, "ClientFileString" },
        { false, FT_STRING, "Name" },
        { false, FT_STRING, "NameFemale" },
        { false, FT_STRING, "NameLowercase" },
        { false, FT_STRING, "NameFemaleLowercase" },
        { false, FT_STRING, "LoreName" },
        { false, FT_STRING, "LoreNameFemale" },
        { false, FT_STRING, "LoreNameLower" },
        { false, FT_STRING, "LoreNameLowerFemale" },
        { false, FT_STRING, "LoreDescription" },
        { false, FT_STRING, "ShortName" },
        { false, FT_STRING, "ShortNameFemale" },
        { false, FT_STRING, "ShortNameLower" },
        { false, FT_STRING, "ShortNameLowerFemale" },
        { true, FT_INT, "Flags" },
        { false, FT_INT, "MaleDisplayID" },
        { false, FT_INT, "FemaleDisplayID" },
        { false, FT_INT, "HighResMaleDisplayID" },
        { false, FT_INT, "HighResFemaleDisplayID" },
        { true, FT_INT, "ResSicknessSpellID" },
        { true, FT_INT, "SplashSoundID" },
        { true, FT_INT, "CreateScreenFileDataID" },
        { true, FT_INT, "SelectScreenFileDataID" },
        { true, FT_INT, "LowResScreenFileDataID" },
        { false, FT_INT, "AlteredFormStartVisualKitID1" },
        { false, FT_INT, "AlteredFormStartVisualKitID2" },
        { false, FT_INT, "AlteredFormStartVisualKitID3" },
        { false, FT_INT, "AlteredFormFinishVisualKitID1" },
        { false, FT_INT, "AlteredFormFinishVisualKitID2" },
        { false, FT_INT, "AlteredFormFinishVisualKitID3" },
        { true, FT_INT, "HeritageArmorAchievementID" },
        { true, FT_INT, "StartingLevel" },
        { true, FT_INT, "UiDisplayOrder" },
        { true, FT_INT, "PlayableRaceBit" },
        { true, FT_INT, "FemaleSkeletonFileDataID" },
        { true, FT_INT, "MaleSkeletonFileDataID" },
        { true, FT_INT, "HelmetAnimScalingRaceID" },
        { true, FT_INT, "TransmogrifyDisabledSlotMask" },
        { false, FT_FLOAT, "AlteredFormCustomizeOffsetFallback1" },
        { false, FT_FLOAT, "AlteredFormCustomizeOffsetFallback2" },
        { false, FT_FLOAT, "AlteredFormCustomizeOffsetFallback3" },
        { false, FT_FLOAT, "AlteredFormCustomizeRotationFallback" },
        { false, FT_FLOAT, "Unknown910_11" },
        { false, FT_FLOAT, "Unknown910_12" },
        { false, FT_FLOAT, "Unknown910_13" },
        { false, FT_FLOAT, "Unknown910_21" },
        { false, FT_FLOAT, "Unknown910_22" },
        { false, FT_FLOAT, "Unknown910_23" },
        { true, FT_SHORT, "FactionID" },
        { true, FT_SHORT, "CinematicSequenceID" },
        { true, FT_BYTE, "BaseLanguage" },
        { true, FT_BYTE, "CreatureType" },
        { true, FT_BYTE, "Alliance" },
        { true, FT_BYTE, "RaceRelated" },
        { true, FT_BYTE, "UnalteredVisualRaceID" },
        { true, FT_BYTE, "DefaultClassID" },
        { true, FT_BYTE, "NeutralRaceID" },
        { true, FT_BYTE, "MaleModelFallbackRaceID" },
        { true, FT_BYTE, "MaleModelFallbackSex" },
        { true, FT_BYTE, "FemaleModelFallbackRaceID" },
        { true, FT_BYTE, "FemaleModelFallbackSex" },
        { true, FT_BYTE, "MaleTextureFallbackRaceID" },
        { true, FT_BYTE, "MaleTextureFallbackSex" },
        { true, FT_BYTE, "FemaleTextureFallbackRaceID" },
        { true, FT_BYTE, "FemaleTextureFallbackSex" },
        { true, FT_BYTE, "UnalteredVisualCustomizationRaceID" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 68, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct ChrClassesLoadInfo
{
    static constexpr DB2MetaField MetaFields[25] =
    {
        { FT_STRING, 1, true },
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1361031, .IndexField = 5, .ParentIndexField = -1,
        .FieldCount = 25, .FileFieldCount = 25, .LayoutHash = 0x3F74F8D7, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[25] =
    {
        { false, FT_STRING, "Name" },
        { false, FT_STRING_NOT_LOCALIZED, "Filename" },
        { false, FT_STRING, "NameMale" },
        { false, FT_STRING, "NameFemale" },
        { false, FT_STRING_NOT_LOCALIZED, "PetNameToken" },
        { false, FT_INT, "ID" },
        { false, FT_INT, "CreateScreenFileDataID" },
        { false, FT_INT, "SelectScreenFileDataID" },
        { false, FT_INT, "IconFileDataID" },
        { false, FT_INT, "LowResScreenFileDataID" },
        { true, FT_INT, "Flags" },
        { true, FT_INT, "StartingLevel" },
        { false, FT_INT, "ArmorTypeMask" },
        { false, FT_SHORT, "CinematicSequenceID" },
        { false, FT_SHORT, "DefaultSpec" },
        { false, FT_BYTE, "HasStrengthAttackBonus" },
        { false, FT_BYTE, "PrimaryStatPriority" },
        { false, FT_BYTE, "DisplayPower" },
        { false, FT_BYTE, "RangedAttackPowerPerAgility" },
        { false, FT_BYTE, "AttackPowerPerAgility" },
        { false, FT_BYTE, "AttackPowerPerStrength" },
        { false, FT_BYTE, "SpellClassSet" },
        { false, FT_BYTE, "RolesMask" },
        { false, FT_BYTE, "DamageBonusStat" },
        { false, FT_BYTE, "HasRelicSlot" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 25, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct SkillLineLoadInfo
{
    static constexpr DB2MetaField MetaFields[13] =
    {
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_INT, 1, false },
        { FT_BYTE, 1, true },
        { FT_INT, 1, true },
        { FT_BYTE, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1240935, .IndexField = 5, .ParentIndexField = -1,
        .FieldCount = 13, .FileFieldCount = 13, .LayoutHash = 0x5CB7F941, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[13] =
    {
        { false, FT_STRING, "DisplayName" },
        { false, FT_STRING, "AlternateVerb" },
        { false, FT_STRING, "Description" },
        { false, FT_STRING, "HordeDisplayName" },
        { false, FT_STRING_NOT_LOCALIZED, "OverrideSourceInfoDisplayName" },
        { false, FT_INT, "ID" },
        { true, FT_BYTE, "CategoryID" },
        { true, FT_INT, "SpellIconFileID" },
        { true, FT_BYTE, "CanLink" },
        { false, FT_INT, "ParentSkillLineID" },
        { true, FT_INT, "ParentTierIndex" },
        { false, FT_SHORT, "Flags" },
        { true, FT_INT, "SpellBookSpellID" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 13, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct SkillLineAbilityLoadInfo
{
    static constexpr DB2MetaField MetaFields[16] =
    {
        { FT_LONG, 1, true },
        { FT_INT, 1, false },
        { FT_SHORT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_BYTE, 1, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_INT, 2, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1266278, .IndexField = 1, .ParentIndexField = 2,
        .FieldCount = 16, .FileFieldCount = 16, .LayoutHash = 0x5DEA6909, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[17] =
    {
        { true, FT_LONG, "RaceMask" },
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "SkillLine" },                          // ParentIndexField -> unsigned
        { true, FT_INT, "Spell" },
        { true, FT_SHORT, "MinSkillLineRank" },
        { true, FT_INT, "ClassMask" },
        { true, FT_INT, "SupercedesSpell" },
        { true, FT_BYTE, "AcquireMethod" },
        { true, FT_SHORT, "TrivialSkillLineRankHigh" },
        { true, FT_SHORT, "TrivialSkillLineRankLow" },
        { true, FT_BYTE, "Flags" },
        { true, FT_BYTE, "NumSkillUps" },
        { true, FT_SHORT, "UniqueBit" },
        { true, FT_SHORT, "TradeSkillCategoryID" },
        { true, FT_SHORT, "SkillupSkillLineID" },
        { true, FT_INT, "CharacterPoints1" },
        { true, FT_INT, "CharacterPoints2" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 17, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct WorldMapOverlayLoadInfo
{
    static constexpr DB2MetaField MetaFields[13] =
    {
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 4, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1134579, .IndexField = 0, .ParentIndexField = 1,
        .FieldCount = 13, .FileFieldCount = 13, .LayoutHash = 0xD73DE991, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[16] =
    {
        { false, FT_INT, "ID" },
        { false, FT_INT, "UiMapArtID" },                          // ParentIndexField -> unsigned
        { false, FT_SHORT, "TextureWidth" },
        { false, FT_SHORT, "TextureHeight" },
        { true, FT_INT, "OffsetX" },
        { true, FT_INT, "OffsetY" },
        { true, FT_INT, "HitRectTop" },
        { true, FT_INT, "HitRectBottom" },
        { true, FT_INT, "HitRectLeft" },
        { true, FT_INT, "HitRectRight" },
        { false, FT_INT, "PlayerConditionID" },
        { false, FT_INT, "Flags" },
        { false, FT_INT, "AreaID1" },
        { false, FT_INT, "AreaID2" },
        { false, FT_INT, "AreaID3" },
        { false, FT_INT, "AreaID4" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 16, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct SkillRaceClassInfoLoadInfo
{
    static constexpr DB2MetaField MetaFields[7] =
    {
        { FT_LONG, 1, true },
        { FT_SHORT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, true },
        { FT_SHORT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1240406, .IndexField = -1, .ParentIndexField = 1,
        .FieldCount = 7, .FileFieldCount = 7, .LayoutHash = 0x0271228C, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[8] =
    {
        { false, FT_INT, "ID" },
        { true, FT_LONG, "RaceMask" },
        { false, FT_SHORT, "SkillID" },                           // ParentIndexField -> unsigned
        { true, FT_INT, "ClassMask" },
        { false, FT_SHORT, "Flags" },
        { true, FT_BYTE, "Availability" },
        { true, FT_BYTE, "MinLevel" },
        { true, FT_SHORT, "SkillTierID" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 8, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct CharTitlesLoadInfo
{
    static constexpr DB2MetaField MetaFields[4] =
    {
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1349054, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 4, .FileFieldCount = 4, .LayoutHash = 0xD7398A05, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[5] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING, "Name" },
        { false, FT_STRING, "Name1" },
        { true, FT_SHORT, "MaskID" },
        { true, FT_BYTE, "Flags" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 5, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct LightLoadInfo
{
    static constexpr DB2MetaField MetaFields[5] =
    {
        { FT_FLOAT, 3, true },
        { FT_FLOAT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 8, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1375579, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 5, .FileFieldCount = 5, .LayoutHash = 0xAD1B2253, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[15] =
    {
        { false, FT_INT, "ID" },
        { false, FT_FLOAT, "GameCoordsX" },
        { false, FT_FLOAT, "GameCoordsY" },
        { false, FT_FLOAT, "GameCoordsZ" },
        { false, FT_FLOAT, "GameFalloffStart" },
        { false, FT_FLOAT, "GameFalloffEnd" },
        { true, FT_SHORT, "ContinentID" },
        { false, FT_SHORT, "LightParamsID1" },
        { false, FT_SHORT, "LightParamsID2" },
        { false, FT_SHORT, "LightParamsID3" },
        { false, FT_SHORT, "LightParamsID4" },
        { false, FT_SHORT, "LightParamsID5" },
        { false, FT_SHORT, "LightParamsID6" },
        { false, FT_SHORT, "LightParamsID7" },
        { false, FT_SHORT, "LightParamsID8" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 15, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct PowerDisplayLoadInfo
{
    static constexpr DB2MetaField MetaFields[5] =
    {
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1332557, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 5, .FileFieldCount = 5, .LayoutHash = 0xE9B4E78C, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[6] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING_NOT_LOCALIZED, "GlobalStringBaseTag" },
        { false, FT_BYTE, "ActualType" },
        { false, FT_BYTE, "Red" },
        { false, FT_BYTE, "Green" },
        { false, FT_BYTE, "Blue" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 6, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct TaxiNodesLoadInfo
{
    static constexpr DB2MetaField MetaFields[14] =
    {
        { FT_STRING, 1, true },
        { FT_FLOAT, 3, true },
        { FT_FLOAT, 2, true },
        { FT_FLOAT, 2, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },                                     // ContinentID -> ParentIndexField, must be unsigned
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_FLOAT, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 2, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1068100, .IndexField = 4, .ParentIndexField = 5,
        .FieldCount = 14, .FileFieldCount = 14, .LayoutHash = 0x609F20BF, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[19] =
    {
        { false, FT_STRING, "Name" },
        { false, FT_FLOAT, "PosX" },
        { false, FT_FLOAT, "PosY" },
        { false, FT_FLOAT, "PosZ" },
        { false, FT_FLOAT, "MapOffsetX" },
        { false, FT_FLOAT, "MapOffsetY" },
        { false, FT_FLOAT, "FlightMapOffsetX" },
        { false, FT_FLOAT, "FlightMapOffsetY" },
        { false, FT_INT, "ID" },
        { false, FT_INT, "ContinentID" },
        { false, FT_INT, "ConditionID" },
        { false, FT_SHORT, "CharacterBitNumber" },
        { true, FT_INT, "Flags" },
        { true, FT_INT, "UiTextureKitID" },
        { false, FT_FLOAT, "Facing" },
        { false, FT_INT, "SpecialIconConditionID" },
        { false, FT_INT, "VisibilityConditionID" },
        { true, FT_INT, "MountCreatureID1" },
        { true, FT_INT, "MountCreatureID2" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 19, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct TaxiPathLoadInfo
{
    static constexpr DB2MetaField MetaFields[4] =
    {
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },                                   // FromTaxiNode -> ParentIndexField, must be unsigned
        { FT_SHORT, 1, false },
        { FT_INT, 1, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1067802, .IndexField = 0, .ParentIndexField = 1,
        .FieldCount = 4, .FileFieldCount = 4, .LayoutHash = 0x9B67699C, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[4] =
    {
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "FromTaxiNode" },
        { false, FT_SHORT, "ToTaxiNode" },
        { false, FT_INT, "Cost" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 4, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct TaxiPathNodeLoadInfo
{
    static constexpr DB2MetaField MetaFields[9] =
    {
        { FT_FLOAT, 3, true },
        { FT_INT, 1, false },
        { FT_SHORT, 1, false },                                   // PathID -> ParentIndexField, must be unsigned
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
        { FT_INT, 1, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1000437, .IndexField = 1, .ParentIndexField = 2,
        .FieldCount = 9, .FileFieldCount = 9, .LayoutHash = 0xC38748B1, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[11] =
    {
        { false, FT_FLOAT, "LocX" },
        { false, FT_FLOAT, "LocY" },
        { false, FT_FLOAT, "LocZ" },
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "PathID" },
        { true, FT_INT, "NodeIndex" },
        { false, FT_SHORT, "ContinentID" },
        { true, FT_INT, "Flags" },
        { false, FT_INT, "Delay" },
        { false, FT_INT, "ArrivalEventID" },
        { false, FT_INT, "DepartureEventID" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 11, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct AreaTableLoadInfo
{
    // Build 3.4.3.54261 (xian55 AreaTableMeta, LayoutHash 0x19CA1DC6). IndexField -1 = ID is the
    // implicit (non-inline) record id: present in the flattened Fields list but NOT in MetaFields.
    static constexpr DB2MetaField MetaFields[23] =
    {
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_STRING, 1, true },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, true },
        { FT_SHORT, 1, false },
        { FT_INT, 1, false },
        { FT_BYTE, 1, false },
        { FT_FLOAT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 2, true },
        { FT_SHORT, 4, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1353545, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 23, .FileFieldCount = 23, .LayoutHash = 0x19CA1DC6, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[28] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING_NOT_LOCALIZED, "ZoneName" },
        { false, FT_STRING, "AreaName" },
        { false, FT_SHORT, "ContinentID" },
        { false, FT_SHORT, "ParentAreaID" },
        { true, FT_SHORT, "AreaBit" },
        { false, FT_BYTE, "SoundProviderPref" },
        { false, FT_BYTE, "SoundProviderPrefUnderwater" },
        { false, FT_SHORT, "AmbienceID" },
        { false, FT_SHORT, "UwAmbience" },
        { false, FT_SHORT, "ZoneMusic" },
        { false, FT_SHORT, "UwZoneMusic" },
        { true, FT_BYTE, "ExplorationLevel" },
        { false, FT_SHORT, "IntroSound" },
        { false, FT_INT, "UwIntroSound" },
        { false, FT_BYTE, "FactionGroupMask" },
        { false, FT_FLOAT, "AmbientMultiplier" },
        { true, FT_INT, "MountFlags" },
        { true, FT_SHORT, "PvpCombatWorldStateID" },
        { false, FT_BYTE, "WildBattlePetLevelMin" },
        { false, FT_BYTE, "WildBattlePetLevelMax" },
        { false, FT_BYTE, "WindSettingsID" },
        { true, FT_INT, "Flags1" },
        { true, FT_INT, "Flags2" },
        { false, FT_SHORT, "LiquidTypeID1" },
        { false, FT_SHORT, "LiquidTypeID2" },
        { false, FT_SHORT, "LiquidTypeID3" },
        { false, FT_SHORT, "LiquidTypeID4" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 28, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct MapLoadInfo
{
    // Build 3.4.3.54261 (xian55 MapMeta, LayoutHash 0xBFC078A9). IndexField -1 = ID is the
    // implicit (non-inline) record id: present in the flattened Fields list but NOT in MetaFields.
    static constexpr DB2MetaField MetaFields[22] =
    {
        { FT_STRING_NOT_LOCALIZED, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, true },
        { FT_BYTE, 1, false },
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, false },
        { FT_FLOAT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 1, true },
        { FT_BYTE, 1, false },
        { FT_SHORT, 1, true },
        { FT_INT, 1, true },
        { FT_INT, 3, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1349477, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 22, .FileFieldCount = 22, .LayoutHash = 0xBFC078A9, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[25] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING_NOT_LOCALIZED, "Directory" },
        { false, FT_STRING, "MapName" },
        { false, FT_STRING, "MapDescription0" },
        { false, FT_STRING, "MapDescription1" },
        { false, FT_STRING, "PvpShortDescription" },
        { false, FT_STRING, "PvpLongDescription" },
        { false, FT_BYTE, "MapType" },
        { true, FT_BYTE, "InstanceType" },
        { false, FT_BYTE, "ExpansionID" },
        { false, FT_SHORT, "AreaTableID" },
        { true, FT_SHORT, "LoadingScreenID" },
        { true, FT_SHORT, "TimeOfDayOverride" },
        { true, FT_SHORT, "ParentMapID" },
        { true, FT_SHORT, "CosmeticParentMapID" },
        { false, FT_BYTE, "TimeOffset" },
        { false, FT_FLOAT, "MinimapIconScale" },
        { true, FT_INT, "RaidOffset" },
        { true, FT_SHORT, "CorpseMapID" },
        { false, FT_BYTE, "MaxPlayers" },
        { true, FT_SHORT, "WindSettingsID" },
        { true, FT_INT, "ZmpFileDataID" },
        { true, FT_INT, "Flags1" },
        { true, FT_INT, "Flags2" },
        { true, FT_INT, "Flags3" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 25, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct MapDifficultyLoadInfo
{
    // Build 3.4.3.54261 (xian55 MapDifficultyMeta, LayoutHash 0x0387F43D). IndexField -1 = ID is the
    // implicit (non-inline) record id. ParentIndexField 9 = MapID (appended, not in the file fields).
    static constexpr DB2MetaField MetaFields[10] =
    {
        { FT_STRING, 1, true },
        { FT_INT, 1, false },
        { FT_INT, 1, true },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, true },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1367868, .IndexField = -1, .ParentIndexField = 9,
        .FieldCount = 10, .FileFieldCount = 9, .LayoutHash = 0x0387F43D, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[11] =
    {
        { false, FT_INT, "ID" },
        { false, FT_STRING, "Message" },
        { false, FT_INT, "ItemContextPickerID" },
        { true, FT_INT, "ContentTuningID" },
        { false, FT_BYTE, "DifficultyID" },
        { false, FT_BYTE, "LockID" },
        { false, FT_BYTE, "ResetInterval" },
        { false, FT_BYTE, "MaxPlayers" },
        { false, FT_BYTE, "ItemContext" },
        { false, FT_BYTE, "Flags" },
        { false, FT_INT, "MapID" },                               // ParentIndexField -> unsigned
    };

    static constexpr DB2LoadInfo Instance{ Fields, 11, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct FactionLoadInfo
{
    static constexpr DB2MetaField MetaFields[18] =
    {
        { FT_LONG, 4, true },
        { FT_STRING, 1, true },
        { FT_STRING, 1, true },
        { FT_INT, 1, false },                                     // ID -> IndexField (3), must be unsigned
        { FT_SHORT, 1, true },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_INT, 1, true },
        { FT_SHORT, 1, false },
        { FT_INT, 1, true },
        { FT_INT, 1, true },
        { FT_SHORT, 4, true },
        { FT_SHORT, 4, false },
        { FT_INT, 4, true },
        { FT_INT, 4, true },
        { FT_FLOAT, 2, true },
        { FT_BYTE, 2, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1361972, .IndexField = 3, .ParentIndexField = -1,
        .FieldCount = 18, .FileFieldCount = 18, .LayoutHash = 0x767B5394, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[35] =
    {
        { true, FT_LONG, "ReputationRaceMask1" },
        { true, FT_LONG, "ReputationRaceMask2" },
        { true, FT_LONG, "ReputationRaceMask3" },
        { true, FT_LONG, "ReputationRaceMask4" },
        { false, FT_STRING, "Name" },
        { false, FT_STRING, "Description" },
        { false, FT_INT, "ID" },
        { true, FT_SHORT, "ReputationIndex" },
        { false, FT_SHORT, "ParentFactionID" },
        { false, FT_BYTE, "Expansion" },
        { false, FT_BYTE, "FriendshipRepID" },
        { true, FT_INT, "Flags" },
        { false, FT_SHORT, "ParagonFactionID" },
        { true, FT_INT, "RenownFactionID" },
        { true, FT_INT, "RenownCurrencyID" },
        { true, FT_SHORT, "ReputationClassMask1" },
        { true, FT_SHORT, "ReputationClassMask2" },
        { true, FT_SHORT, "ReputationClassMask3" },
        { true, FT_SHORT, "ReputationClassMask4" },
        { false, FT_SHORT, "ReputationFlags1" },
        { false, FT_SHORT, "ReputationFlags2" },
        { false, FT_SHORT, "ReputationFlags3" },
        { false, FT_SHORT, "ReputationFlags4" },
        { true, FT_INT, "ReputationBase1" },
        { true, FT_INT, "ReputationBase2" },
        { true, FT_INT, "ReputationBase3" },
        { true, FT_INT, "ReputationBase4" },
        { true, FT_INT, "ReputationMax1" },
        { true, FT_INT, "ReputationMax2" },
        { true, FT_INT, "ReputationMax3" },
        { true, FT_INT, "ReputationMax4" },
        { false, FT_FLOAT, "ParentFactionMod1" },
        { false, FT_FLOAT, "ParentFactionMod2" },
        { false, FT_BYTE, "ParentFactionCap1" },
        { false, FT_BYTE, "ParentFactionCap2" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 35, &MetaInstance, HotfixDatabaseStatements(0) };
};

struct FactionTemplateLoadInfo
{
    static constexpr DB2MetaField MetaFields[7] =
    {
        { FT_SHORT, 1, false },
        { FT_SHORT, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_BYTE, 1, false },
        { FT_SHORT, 8, false },
        { FT_SHORT, 8, false },
    };

    static constexpr DB2Meta MetaInstance =
    {
        .FileDataId = 1361579, .IndexField = -1, .ParentIndexField = -1,
        .FieldCount = 7, .FileFieldCount = 7, .LayoutHash = 0x207C5E80, .Fields = MetaFields
    };

    static constexpr DB2FieldMeta Fields[22] =
    {
        { false, FT_INT, "ID" },
        { false, FT_SHORT, "Faction" },
        { false, FT_SHORT, "Flags" },
        { false, FT_BYTE, "FactionGroup" },
        { false, FT_BYTE, "FriendGroup" },
        { false, FT_BYTE, "EnemyGroup" },
        { false, FT_SHORT, "Enemies1" },
        { false, FT_SHORT, "Enemies2" },
        { false, FT_SHORT, "Enemies3" },
        { false, FT_SHORT, "Enemies4" },
        { false, FT_SHORT, "Enemies5" },
        { false, FT_SHORT, "Enemies6" },
        { false, FT_SHORT, "Enemies7" },
        { false, FT_SHORT, "Enemies8" },
        { false, FT_SHORT, "Friend1" },
        { false, FT_SHORT, "Friend2" },
        { false, FT_SHORT, "Friend3" },
        { false, FT_SHORT, "Friend4" },
        { false, FT_SHORT, "Friend5" },
        { false, FT_SHORT, "Friend6" },
        { false, FT_SHORT, "Friend7" },
        { false, FT_SHORT, "Friend8" },
    };

    static constexpr DB2LoadInfo Instance{ Fields, 22, &MetaInstance, HotfixDatabaseStatements(0) };
};

#endif // AC_DB2LOADINFO_H
