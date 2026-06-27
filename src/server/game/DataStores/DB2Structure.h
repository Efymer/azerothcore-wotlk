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

// Runtime DB2 record structures for build 3.4.3.54261. The whole block is
// #pragma pack(push, 1): the DB2 loader produces packed records (stride =
// DB2Meta::GetRecordSize(), no inter-field padding) and LookupEntry()
// reinterpret_casts each record pointer to T, so T must be packed and match
// TrinityCore's wotlk_classic (xian55) field order byte-for-byte. The boot-time
// sizeof(T) == GetRecordSize() check in DB2Stores.cpp guards against drift.
//
// NOTE: stores whose names collide with the legacy DBC set (MapEntry/sMapStore,
// AreaTableEntry, ChrRacesEntry, ...) are added as part of the DBC->DB2 repoint
// (Task 1c.3), where the DBC version is removed in the same step. This file
// currently carries DB2-only stores that have no DBC twin.
//
// RaceMask fields are modelled as int64 (FT_LONG, 8 bytes) — the typed
// Acore::RaceMask wrapper is unnecessary until a consumer needs the helpers.

#pragma pack(push, 1)

struct LiquidMaterialEntry
{
    uint32 ID;
    int8 Flags;
    int8 LVF;
};

struct SpellNameEntry
{
    uint32 ID;                                                     // SpellID
    LocalizedString Name;
};

struct CharacterLoadoutEntry
{
    int64 RaceMask;
    uint32 ID;
    int8 ChrClassID;
    int32 Purpose;
    int8 ItemContext;
};

struct CharacterLoadoutItemEntry
{
    uint32 ID;
    uint16 CharacterLoadoutID;
    uint32 ItemID;
};

struct ChrCustomizationOptionEntry
{
    LocalizedString Name;
    uint32 ID;
    uint16 SecondaryID;
    int32 Flags;
    int32 ChrModelID;
    int32 SortIndex;
    int32 ChrCustomizationCategoryID;
    int32 OptionType;
    float BarberShopCostModifier;
    int32 ChrCustomizationID;
    int32 ChrCustomizationReqID;
    int32 UiOrderIndex;
};

struct ChrCustomizationReqEntry
{
    int64 RaceMask;
    LocalizedString ReqSource;
    uint32 ID;
    int32 Flags;
    int32 ClassMask;
    int32 AchievementID;
    int32 QuestID;
    int32 OverrideArchive;                                        // -1: allow any, else must match OverrideArchive cvar
    int32 ItemModifiedAppearanceID;
};

struct ItemEffectEntry
{
    uint32 ID;
    uint8 LegacySlotIndex;
    int8 TriggerType;
    int16 Charges;
    int32 CoolDownMSec;
    int32 CategoryCoolDownMSec;
    uint16 SpellCategoryID;
    int32 SpellID;
    uint16 ChrSpecializationID;
    uint32 ParentItemID;
};

struct ItemAppearanceEntry
{
    uint32 ID;
    uint8 DisplayType;
    int32 ItemDisplayInfoID;
    int32 DefaultIconFileDataID;
    int32 UiOrder;
};

struct ItemModifiedAppearanceEntry
{
    uint32 ID;
    int32 ItemID;
    int32 ItemAppearanceModifierID;
    int32 ItemAppearanceID;
    int32 OrderIndex;
    int32 TransmogSourceTypeEnum;
};

struct PowerTypeEntry
{
    uint32 ID;
    char const* NameGlobalStringTag;
    char const* CostGlobalStringTag;
    int8 PowerTypeEnum;
    int32 MinPower;
    int32 MaxBasePower;
    int32 CenterPower;
    int32 DefaultPower;
    int32 DisplayModifier;
    int32 RegenInterruptTimeMS;
    float RegenPeace;
    float RegenCombat;
    int16 Flags;
};

struct ItemSparseEntry
{
    uint32 ID;
    int64 AllowableRace;
    LocalizedString Description;
    LocalizedString Display3;
    LocalizedString Display2;
    LocalizedString Display1;
    LocalizedString Display;
    float DmgVariance;
    uint32 DurationInInventory;
    float QualityModifier;
    uint32 BagFamily;
    int32 StartQuestID;
    float ItemRange;
    std::array<float, 10> StatPercentageOfSocket;
    std::array<int32, 10> StatPercentEditor;
    int32 Stackable;
    int32 MaxCount;
    int32 MinReputation;
    uint32 RequiredAbility;
    uint32 SellPrice;
    uint32 BuyPrice;
    uint32 VendorStackCount;
    float PriceVariance;
    float PriceRandomValue;
    std::array<int32, 4> Flags;
    int32 FactionRelated;
    int32 ModifiedCraftingReagentItemID;
    int32 ContentTuningID;
    int32 PlayerLevelToItemLevelCurveID;
    uint32 MaxDurability;
    uint16 ItemNameDescriptionID;
    uint16 RequiredTransmogHoliday;
    uint16 RequiredHoliday;
    uint16 LimitCategory;
    uint16 GemProperties;
    uint16 SocketMatchEnchantmentId;
    uint16 TotemCategoryID;
    uint16 InstanceBound;
    std::array<uint16, 2> ZoneBound;
    uint16 ItemSet;
    uint16 LockID;
    uint16 PageID;
    uint16 ItemDelay;
    uint16 MinFactionID;
    uint16 RequiredSkillRank;
    uint16 RequiredSkill;
    uint16 ItemLevel;
    int16 AllowableClass;
    uint16 ItemRandomSuffixGroupID;
    uint16 RandomSelect;
    std::array<uint16, 5> MinDamage;
    std::array<uint16, 5> MaxDamage;
    std::array<int16, 7> Resistances;
    uint16 ScalingStatDistributionID;
    std::array<int16, 10> StatModifierBonusAmount;
    uint8 ExpansionID;
    uint8 ArtifactID;
    uint8 SpellWeight;
    uint8 SpellWeightCategory;
    std::array<uint8, 3> SocketType;
    uint8 SheatheType;
    uint8 Material;
    uint8 PageMaterialID;
    uint8 LanguageID;
    uint8 Bonding;
    uint8 DamageDamageType;
    std::array<int8, 10> StatModifierBonusStat;
    uint8 ContainerSlots;
    uint8 RequiredPVPMedal;
    uint8 RequiredPVPRank;
    int8 InventoryType;
    int8 OverallQualityID;
    uint8 AmmunitionType;
    int8 RequiredLevel;
};

#pragma pack(pop)

#endif // AC_DB2STRUCTURE_H
