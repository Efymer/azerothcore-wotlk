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

#include "AreaDefines.h"
#include "Common.h"
#include "DB2FileLoader.h"
#include "DBCEnums.h"
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

// Migrated from DBC (3.4.3.54261). Legacy DBC field map:
//   mapid->ContinentID, zone->ParentAreaID, exploreFlag->AreaBit, flags->Flags[0],
//   area_level->ExplorationLevel, area_name[loc]->AreaName.Str[loc], team->FactionGroupMask,
//   LiquidTypeOverride[i]->LiquidTypeID[i]. Legacy helpers IsSanctuary()/IsFlyable() preserved.
struct AreaTableEntry
{
    uint32 ID;
    char const* ZoneName;
    LocalizedString AreaName;
    uint16 ContinentID;
    uint16 ParentAreaID;
    int16 AreaBit;
    uint8 SoundProviderPref;
    uint8 SoundProviderPrefUnderwater;
    uint16 AmbienceID;
    uint16 UwAmbience;
    uint16 ZoneMusic;
    uint16 UwZoneMusic;
    int8 ExplorationLevel;
    uint16 IntroSound;
    uint32 UwIntroSound;
    uint8 FactionGroupMask;
    float AmbientMultiplier;
    int32 MountFlags;
    int16 PvpCombatWorldStateID;
    uint8 WildBattlePetLevelMin;
    uint8 WildBattlePetLevelMax;
    uint8 WindSettingsID;
    std::array<int32, 2> Flags;
    std::array<uint16, 4> LiquidTypeID;

    // helpers (legacy API preserved; field names repointed to DB2)
    [[nodiscard]] bool IsSanctuary() const
    {
        if (ContinentID == MAP_EBON_HOLD)
            return true;
        return (Flags[0] & AREA_FLAG_SANCTUARY) != 0;
    }

    [[nodiscard]] bool IsFlyable() const
    {
        // 3.4.3.54261: flyability is carried by the dedicated MountFlags field
        // (AreaMountFlags::AllowFlyingMounts = 0x2), NOT the legacy 3.3.5a heuristic
        // `Flags & AREA_FLAG_OUTLAND` — in WDC4 that 0x400 bit was repurposed
        // (ForceThisAreaWhenOnDynamicTransport). Mirrors xian55 GetMountFlags().
        return (MountFlags & 0x2) != 0;
    }
};

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

struct PlayerConditionEntry
{
    int64 RaceMask;
    LocalizedString FailureDescription;
    uint32 ID;
    uint16 MinLevel;
    uint16 MaxLevel;
    int32 ClassMask;
    uint32 SkillLogic;
    uint8 LanguageID;
    uint8 MinLanguage;
    int32 MaxLanguage;
    uint16 MaxFactionID;
    uint8 MaxReputation;
    uint32 ReputationLogic;
    int8 CurrentPvpFaction;
    uint8 PvpMedal;
    uint32 PrevQuestLogic;
    uint32 CurrQuestLogic;
    uint32 CurrentCompletedQuestLogic;
    uint32 SpellLogic;
    uint32 ItemLogic;
    uint8 ItemFlags;
    uint32 AuraSpellLogic;
    uint16 WorldStateExpressionID;
    uint8 WeatherID;
    uint8 PartyStatus;
    uint8 LifetimeMaxPVPRank;
    uint32 AchievementLogic;
    int8 Gender;
    int8 NativeGender;
    uint32 AreaLogic;
    uint32 LfgLogic;
    uint32 CurrencyLogic;
    uint32 QuestKillID;
    uint32 QuestKillLogic;
    int8 MinExpansionLevel;
    int8 MaxExpansionLevel;
    int32 MinAvgItemLevel;
    int32 MaxAvgItemLevel;
    uint16 MinAvgEquippedItemLevel;
    uint16 MaxAvgEquippedItemLevel;
    uint8 PhaseUseFlags;
    uint16 PhaseID;
    uint32 PhaseGroupID;
    uint8 Flags;
    int8 ChrSpecializationIndex;
    int8 ChrSpecializationRole;
    uint32 ModifierTreeID;
    int8 PowerType;
    uint8 PowerTypeComp;
    uint8 PowerTypeValue;
    int32 WeaponSubclassMask;
    uint8 MaxGuildLevel;
    uint8 MinGuildLevel;
    int8 MaxExpansionTier;
    int8 MinExpansionTier;
    uint8 MinPVPRank;
    uint8 MaxPVPRank;
    std::array<uint16, 4> SkillID;
    std::array<uint16, 4> MinSkill;
    std::array<uint16, 4> MaxSkill;
    std::array<uint32, 3> MinFactionID;
    std::array<uint8, 3> MinReputation;
    std::array<uint32, 4> PrevQuestID;
    std::array<uint32, 4> CurrQuestID;
    std::array<uint32, 4> CurrentCompletedQuestID;
    std::array<int32, 4> SpellID;
    std::array<int32, 4> ItemID;
    std::array<uint32, 4> ItemCount;
    std::array<uint16, 2> Explored;
    std::array<uint32, 2> Time;
    std::array<int32, 4> AuraSpellID;
    std::array<uint8, 4> AuraStacks;
    std::array<uint16, 4> Achievement;
    std::array<uint16, 4> AreaID;
    std::array<uint8, 4> LfgStatus;
    std::array<uint8, 4> LfgCompare;
    std::array<uint32, 4> LfgValue;
    std::array<uint32, 4> CurrencyID;
    std::array<uint32, 4> CurrencyCount;
    std::array<uint32, 6> QuestKillMonster;
    std::array<int32, 2> MovementFlags;
};

// Migrated from DBC (Task 1c.3). Legacy DBC field -> DB2 field: powerType -> DisplayPower,
// spellfamily -> SpellClassSet, CinematicSequence -> CinematicSequenceID. The DBC `expansion`
// field has no DB2 equivalent (3.4.3 moves class-by-expansion gating to a DB table).
struct ChrClassesEntry
{
    LocalizedString Name;
    char const* Filename;
    LocalizedString NameMale;
    LocalizedString NameFemale;
    char const* PetNameToken;
    uint32 ID;
    uint32 CreateScreenFileDataID;
    uint32 SelectScreenFileDataID;
    uint32 IconFileDataID;
    uint32 LowResScreenFileDataID;
    int32 Flags;
    int32 StartingLevel;
    uint32 ArmorTypeMask;
    uint16 CinematicSequenceID;
    uint16 DefaultSpec;
    uint8 HasStrengthAttackBonus;
    uint8 PrimaryStatPriority;
    uint8 DisplayPower;
    uint8 RangedAttackPowerPerAgility;
    uint8 AttackPowerPerAgility;
    uint8 AttackPowerPerStrength;
    uint8 SpellClassSet;
    uint8 RolesMask;
    uint8 DamageBonusStat;
    uint8 HasRelicSlot;
};

// Migrated from DBC (Task 1c.3). Legacy DBC -> DB2 field mapping: RaceID->ID, model_m->MaleDisplayID,
// model_f->FemaleDisplayID, TeamID(7/1)->Alliance(0/1), CinematicSequence->CinematicSequenceID,
// name[locale]->Name.Str[locale]. The DBC `expansion` field moved to race_unlock_requirement; the
// "not playable" flag check is replaced by PlayableRaceBit (< 0 == not playable).
struct ChrRacesEntry
{
    uint32 ID;
    char const* ClientPrefix;
    char const* ClientFileString;
    LocalizedString Name;
    LocalizedString NameFemale;
    LocalizedString NameLowercase;
    LocalizedString NameFemaleLowercase;
    LocalizedString LoreName;
    LocalizedString LoreNameFemale;
    LocalizedString LoreNameLower;
    LocalizedString LoreNameLowerFemale;
    LocalizedString LoreDescription;
    LocalizedString ShortName;
    LocalizedString ShortNameFemale;
    LocalizedString ShortNameLower;
    LocalizedString ShortNameLowerFemale;
    int32 Flags;
    uint32 MaleDisplayID;
    uint32 FemaleDisplayID;
    uint32 HighResMaleDisplayID;
    uint32 HighResFemaleDisplayID;
    int32 ResSicknessSpellID;
    int32 SplashSoundID;
    int32 CreateScreenFileDataID;
    int32 SelectScreenFileDataID;
    int32 LowResScreenFileDataID;
    std::array<uint32, 3> AlteredFormStartVisualKitID;
    std::array<uint32, 3> AlteredFormFinishVisualKitID;
    int32 HeritageArmorAchievementID;
    int32 StartingLevel;
    int32 UiDisplayOrder;
    int32 PlayableRaceBit;
    int32 FemaleSkeletonFileDataID;
    int32 MaleSkeletonFileDataID;
    int32 HelmetAnimScalingRaceID;
    int32 TransmogrifyDisabledSlotMask;
    std::array<float, 3> AlteredFormCustomizeOffsetFallback;
    float AlteredFormCustomizeRotationFallback;
    std::array<float, 3> Unknown910_1;
    std::array<float, 3> Unknown910_2;
    int16 FactionID;
    int16 CinematicSequenceID;
    int8 BaseLanguage;
    int8 CreatureType;
    int8 Alliance;
    int8 RaceRelated;
    int8 UnalteredVisualRaceID;
    int8 DefaultClassID;
    int8 NeutralRaceID;
    int8 MaleModelFallbackRaceID;
    int8 MaleModelFallbackSex;
    int8 FemaleModelFallbackRaceID;
    int8 FemaleModelFallbackSex;
    int8 MaleTextureFallbackRaceID;
    int8 MaleTextureFallbackSex;
    int8 FemaleTextureFallbackRaceID;
    int8 FemaleTextureFallbackSex;
    int8 UnalteredVisualCustomizationRaceID;
};

// Migrated from DBC (Task 1c.3). Legacy DBC -> DB2: id->ID, categoryId->CategoryID,
// name[locale]->DisplayName.Str[locale], spellIcon->SpellIconFileID, canLink->CanLink.
struct SkillLineEntry
{
    LocalizedString DisplayName;
    LocalizedString AlternateVerb;
    LocalizedString Description;
    LocalizedString HordeDisplayName;
    char const* OverrideSourceInfoDisplayName;
    uint32 ID;
    int8 CategoryID;
    int32 SpellIconFileID;
    int8 CanLink;
    uint32 ParentSkillLineID;
    int32 ParentTierIndex;
    uint16 Flags;
    int32 SpellBookSpellID;
};

// Migrated from DBC (Task 1c.3). Field names match the DBC entry (SkillLine, Spell, RaceMask,
// ClassMask, AcquireMethod, TrivialSkillLineRank*); only widths change. RaceMask is int64 now.
struct SkillLineAbilityEntry
{
    int64 RaceMask;
    uint32 ID;
    int16 SkillLine;
    int32 Spell;
    int16 MinSkillLineRank;
    int32 ClassMask;
    int32 SupercedesSpell;
    int8 AcquireMethod;
    int16 TrivialSkillLineRankHigh;
    int16 TrivialSkillLineRankLow;
    int8 Flags;
    int8 NumSkillUps;
    int16 UniqueBit;
    int16 TradeSkillCategoryID;
    int16 SkillupSkillLineID;
    std::array<int32, 2> CharacterPoints;
};

// Migrated from DBC (Task 1c.3). DBC areatableID[] -> DB2 AreaID[].
struct WorldMapOverlayEntry
{
    uint32 ID;
    uint32 UiMapArtID;
    uint16 TextureWidth;
    uint16 TextureHeight;
    int32 OffsetX;
    int32 OffsetY;
    int32 HitRectTop;
    int32 HitRectBottom;
    int32 HitRectLeft;
    int32 HitRectRight;
    uint32 PlayerConditionID;
    uint32 Flags;
    std::array<uint32, 4> AreaID;
};

// Migrated from DBC (Task 1c.3). Field names match (SkillID, RaceMask, ClassMask, Flags,
// SkillTierID); RaceMask is int64 now.
struct SkillRaceClassInfoEntry
{
    uint32 ID;
    int64 RaceMask;
    int16 SkillID;
    int32 ClassMask;
    uint16 Flags;
    int8 Availability;
    int8 MinLevel;
    int16 SkillTierID;
};

// Migrated from DBC (Task 1c.3). DBC {ID, nameMale[16], nameFemale[16], bit_index} -> DB2:
// nameMale->Name.Str, nameFemale->Name1.Str, bit_index->MaskID.
struct CharTitlesEntry
{
    uint32 ID;
    LocalizedString Name;
    LocalizedString Name1;
    int16 MaskID;
    int8 Flags;
};

// Migrated from DBC (Task 1c.3). DBC {Id, MapId, X, Y, Z} -> DB2: MapId->ContinentID,
// X/Y/Z->GameCoords.{X,Y,Z}.
struct LightEntry
{
    uint32 ID;
    DBCPosition3D GameCoords;
    float GameFalloffStart;
    float GameFalloffEnd;
    int16 ContinentID;
    std::array<uint16, 8> LightParamsID;
};

// Migrated from DBC (Task 1c.3): the DBC PowerDisplayEntry {Id, PowerType} maps to the
// 54261 DB2 layout; the legacy `PowerType` field is `ActualType` here.
struct PowerDisplayEntry
{
    uint32 ID;
    char const* GlobalStringBaseTag;
    uint8 ActualType;
    uint8 Red;
    uint8 Green;
    uint8 Blue;
};

struct TaxiNodesEntry
{
    LocalizedString Name;
    DBCPosition3D Pos;
    DBCPosition2D MapOffset;
    DBCPosition2D FlightMapOffset;
    uint32 ID;
    uint32 ContinentID;
    uint32 ConditionID;
    uint16 CharacterBitNumber;
    int32 Flags;
    int32 UiTextureKitID;
    float Facing;
    uint32 SpecialIconConditionID;
    uint32 VisibilityConditionID;
    std::array<int32, 2> MountCreatureID;
};

struct TaxiPathEntry
{
    uint32 ID;
    uint16 FromTaxiNode;
    uint16 ToTaxiNode;
    uint32 Cost;
};

struct TaxiPathNodeEntry
{
    DBCPosition3D Loc;
    uint32 ID;
    uint16 PathID;
    int32 NodeIndex;
    uint16 ContinentID;
    int32 Flags;
    uint32 Delay;
    uint32 ArrivalEventID;
    uint32 DepartureEventID;
};

// Migrated from DBC (Faction/FactionTemplate). Legacy DBC -> DB2 field mapping:
//   FactionEntry: reputationListID->ReputationIndex, BaseRepRaceMask->ReputationRaceMask (uint32[4] -> int64[4]),
//     BaseRepClassMask->ReputationClassMask, BaseRepValue->ReputationBase, team->ParentFactionID,
//     spilloverRateIn/Out->ParentFactionMod[0]/[1], spilloverMaxRankIn->ParentFactionCap[0],
//     name[locale]->Name.Str[locale]. ReputationFlags keeps its name.
//   FactionTemplateEntry: faction->Faction, factionFlags->Flags, ourMask->FactionGroup,
//     friendlyMask->FriendGroup, hostileMask->EnemyGroup, enemyFaction->Enemies, friendFaction->Friend.
// MAX_FACTION_RELATIONS grows 4 -> 8 (54261 layout). Legacy helper API is preserved verbatim
// (same names/signatures, including the FactionTemplateEntry const& by-reference args).
struct FactionEntry
{
    std::array<int64, 4> ReputationRaceMask;
    LocalizedString Name;
    LocalizedString Description;
    uint32 ID;
    int16 ReputationIndex;
    uint16 ParentFactionID;
    uint8 Expansion;
    uint8 FriendshipRepID;
    int32 Flags;
    uint16 ParagonFactionID;
    int32 RenownFactionID;
    int32 RenownCurrencyID;
    std::array<int16, 4> ReputationClassMask;
    std::array<uint16, 4> ReputationFlags;
    std::array<int32, 4> ReputationBase;
    std::array<int32, 4> ReputationMax;
    std::array<float, 2> ParentFactionMod;            // Faction gains incoming rep * ParentFactionMod[0]; outputs rep * ParentFactionMod[1] as spillover
    std::array<uint8, 2> ParentFactionCap;            // [0] = highest rank the faction will profit from incoming spillover

    // helpers
    [[nodiscard]] bool CanHaveReputation() const
    {
        return ReputationIndex >= 0;
    }

    [[nodiscard]] bool CanBeSetAtWar() const
    {
        return ReputationIndex >= 0 && ReputationRaceMask[0] == 1791;
    }
};

#define MAX_FACTION_RELATIONS 8

struct FactionTemplateEntry
{
    uint32 ID;
    uint16 Faction;
    uint16 Flags;
    uint8 FactionGroup;
    uint8 FriendGroup;
    uint8 EnemyGroup;
    std::array<uint16, MAX_FACTION_RELATIONS> Enemies;
    std::array<uint16, MAX_FACTION_RELATIONS> Friend;
    //-------------------------------------------------------  end structure

    // helpers
    [[nodiscard]] bool IsFriendlyTo(FactionTemplateEntry const& entry) const
    {
        // Xinef: Always friendly to self faction
        if (Faction == entry.Faction)
            return true;

        if (entry.Faction)
        {
            for (uint16 i : Enemies)
                if (i == entry.Faction)
                    return false;
            for (uint16 i : Friend)
                if (i == entry.Faction)
                    return true;
        }
        return (FriendGroup & entry.FactionGroup) || (FactionGroup & entry.FriendGroup);
    }
    [[nodiscard]] bool IsHostileTo(FactionTemplateEntry const& entry) const
    {
        if (entry.Faction)
        {
            for (uint16 i : Enemies)
                if (i == entry.Faction)
                    return true;
            for (uint16 i : Friend)
                if (i == entry.Faction)
                    return false;
        }
        return (EnemyGroup & entry.FactionGroup) != 0;
    }
    [[nodiscard]] bool IsHostileToPlayers() const { return (EnemyGroup & FACTION_MASK_PLAYER) != 0; }
    [[nodiscard]] bool IsHostileToAlliancePlayers() const { return (EnemyGroup & FACTION_MASK_ALLIANCE) != 0; }
    [[nodiscard]] bool IsHostileToHordePlayers() const { return (EnemyGroup & FACTION_MASK_HORDE) != 0; }
    [[nodiscard]] bool IsNeutralToAll() const
    {
        for (uint16 i : Enemies)
            if (i != 0)
                return false;
        return EnemyGroup == 0 && FriendGroup == 0;
    }
    [[nodiscard]] bool IsContestedGuardFaction() const { return (Flags & FACTION_TEMPLATE_FLAG_ATTACK_PVP_ACTIVE_PLAYERS) != 0; }
    [[nodiscard]] bool FactionRespondsToCallForHelp() const { return (Flags & FACTION_TEMPLATE_FLAG_RESPOND_TO_CALL_FOR_HELP) != 0; }
};

// Migrated from DBC (3.3.5a Map.dbc) to build 3.4.3.54261 Map.db2 (WDC4, LayoutHash 0xBFC078A9).
// Field order/types are byte-for-byte vs xian55 DB2Structure.h MapEntry. Legacy field renames:
//   MapID->ID, map_type->InstanceType, name[loc]->MapName.Str[loc], linked_zone->AreaTableID,
//   addon/expansionID->ExpansionID, maxPlayers->MaxPlayers, entrance_map->CorpseMapID, Flags->Flags[0..2].
// AC lacks the MapFlags EnumFlag wrapper, so Flags is modelled as the raw int triple (3.3.5a helpers
// read the bit directly). The 3.3.5a corpse X/Y coordinates were dropped from the client store.
struct MapEntry
{
    uint32 ID;
    char const* Directory;
    LocalizedString MapName;
    LocalizedString MapDescription0;                               // Horde
    LocalizedString MapDescription1;                               // Alliance
    LocalizedString PvpShortDescription;
    LocalizedString PvpLongDescription;
    uint8 MapType;
    int8 InstanceType;
    uint8 ExpansionID;
    uint16 AreaTableID;
    int16 LoadingScreenID;
    int16 TimeOfDayOverride;
    int16 ParentMapID;
    int16 CosmeticParentMapID;
    uint8 TimeOffset;
    float MinimapIconScale;
    int32 RaidOffset;
    int16 CorpseMapID;                                             // map_id of entrance map in ghost mode (continent always and in most cases = normal entrance)
    uint8 MaxPlayers;
    int16 WindSettingsID;
    int32 ZmpFileDataID;
    std::array<int32, 3> Flags;

    // helpers (legacy 3.3.5a DBC API preserved; field names repointed to DB2)
    [[nodiscard]] uint32 Expansion() const { return ExpansionID; }

    [[nodiscard]] bool IsDungeon() const { return InstanceType == MAP_INSTANCE || InstanceType == MAP_RAID; }
    [[nodiscard]] bool IsNonRaidDungeon() const { return InstanceType == MAP_INSTANCE; }
    [[nodiscard]] bool Instanceable() const { return InstanceType == MAP_INSTANCE || InstanceType == MAP_RAID || InstanceType == MAP_BATTLEGROUND || InstanceType == MAP_ARENA; }
    [[nodiscard]] bool IsRaid() const { return InstanceType == MAP_RAID; }
    [[nodiscard]] bool IsBattleground() const { return InstanceType == MAP_BATTLEGROUND; }
    [[nodiscard]] bool IsBattleArena() const { return InstanceType == MAP_ARENA; }
    [[nodiscard]] bool IsBattlegroundOrArena() const { return InstanceType == MAP_BATTLEGROUND || InstanceType == MAP_ARENA; }
    [[nodiscard]] bool IsWorldMap() const { return InstanceType == MAP_COMMON; }

    bool GetEntrancePos(int32& mapid, float& /*x*/, float& /*y*/) const
    {
        // 3.4.3.54261 Map.db2 carries only the entrance/ghost map id (CorpseMapID); the corpse X/Y
        // coordinates present in the 3.3.5a Map.dbc were dropped from the client store.
        if (CorpseMapID < 0)
            return false;
        mapid = CorpseMapID;
        return true;
    }

    [[nodiscard]] bool IsContinent() const
    {
        return ID == MAP_EASTERN_KINGDOMS || ID == MAP_KALIMDOR || ID == MAP_OUTLAND || ID == MAP_NORTHREND;
    }

    [[nodiscard]] bool IsDynamicDifficultyMap() const { return (Flags[0] & MAP_FLAG_DYNAMIC_DIFFICULTY) != 0; }
};

#pragma pack(pop)

#endif // AC_DB2STRUCTURE_H
