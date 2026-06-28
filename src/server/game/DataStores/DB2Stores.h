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

#ifndef AC_DB2STORES_H
#define AC_DB2STORES_H

#include "Common.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include <array>
#include <compare>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

// World-entry DB2 store subset for build 3.4.3.54261 (Phase 1c). DB2-only stores (no DBC twin)
// live here now; DBC-colliding stores are added during the DBC->DB2 repoint (Task 1c.3).
extern DB2Storage<ChrClassesEntry> sChrClassesStore;            // migrated from DBC (1c.3)
extern DB2Storage<ChrRacesEntry> sChrRacesStore;                // migrated from DBC (1c.3)
extern DB2Storage<SkillLineEntry> sSkillLineStore;             // migrated from DBC (1c.3)
extern DB2Storage<SkillLineAbilityEntry> sSkillLineAbilityStore; // migrated from DBC (1c.3)
extern DB2Storage<LightEntry> sLightStore;                     // migrated from DBC (1c.3)
extern DB2Storage<WorldMapOverlayEntry> sWorldMapOverlayStore; // migrated from DBC (1c.3)
extern DB2Storage<SkillRaceClassInfoEntry> sSkillRaceClassInfoStore; // migrated from DBC (1c.3)
extern DB2Storage<CharTitlesEntry> sCharTitlesStore;           // migrated from DBC (1c.3)
extern DB2Storage<LiquidMaterialEntry> sLiquidMaterialStore;
extern DB2Storage<SpellNameEntry> sSpellNameStore;
extern DB2Storage<CharacterLoadoutEntry> sCharacterLoadoutStore;
extern DB2Storage<CharacterLoadoutItemEntry> sCharacterLoadoutItemStore;
extern DB2Storage<ChrCustomizationOptionEntry> sChrCustomizationOptionStore;
extern DB2Storage<ChrCustomizationReqEntry> sChrCustomizationReqStore;
extern DB2Storage<ChrCustomizationChoiceEntry> sChrCustomizationChoiceStore;
extern DB2Storage<ChrCustomizationDisplayInfoEntry> sChrCustomizationDisplayInfoStore;
extern DB2Storage<ChrCustomizationElementEntry> sChrCustomizationElementStore;
extern DB2Storage<ItemEffectEntry> sItemEffectStore;
extern DB2Storage<ItemAppearanceEntry> sItemAppearanceStore;
extern DB2Storage<ItemModifiedAppearanceEntry> sItemModifiedAppearanceStore;
extern DB2Storage<PowerTypeEntry> sPowerTypeStore;
extern DB2Storage<ItemEntry> sItemStore;                       // migrated from DBC
extern DB2Storage<ItemSparseEntry> sItemSparseStore;
extern DB2Storage<PlayerConditionEntry> sPlayerConditionStore;
extern DB2Storage<PowerDisplayEntry> sPowerDisplayStore;        // migrated from DBC (1c.3)
extern DB2Storage<TaxiNodesEntry> sTaxiNodesStore;              // migrated from DBC (1c.3)
extern DB2Storage<TaxiPathEntry> sTaxiPathStore;               // migrated from DBC (1c.3)
extern DB2Storage<TaxiPathNodeEntry> sTaxiPathNodeStore;       // migrated from DBC (1c.3)
extern DB2Storage<AreaTableEntry> sAreaTableStore;             // migrated from DBC
extern DB2Storage<FactionEntry> sFactionStore;                 // migrated from DBC
extern DB2Storage<FactionTemplateEntry> sFactionTemplateStore; // migrated from DBC
extern DB2Storage<MapEntry> sMapStore;                         // migrated from DBC
extern DB2Storage<MapDifficultyEntry> sMapDifficultyStore;     // migrated from DBC
extern DB2Storage<CinematicCameraEntry> sCinematicCameraStore; // migrated from DBC
extern DB2Storage<CinematicSequencesEntry> sCinematicSequencesStore; // migrated from DBC
extern DB2Storage<CurrencyTypesEntry> sCurrencyTypesStore;     // migrated from DBC

// Minimal DB2Manager: the hotfix-delivery subset ported from the 3.4.3 reference core. AzerothCore does not
// have the full TrinityCore DB2Manager; this provides the table-hash -> store lookup plus the hotfix_data /
// hotfix_blob / hotfix_optional_data caches the hotfix handlers need to answer CMSG_DB_QUERY_BULK and
// CMSG_HOTFIX_REQUEST with real data (an empty manifest makes the 54261 client skip a GCM nonce).
class DB2Manager
{
public:
    struct HotfixId
    {
        int32 PushID = 0;
        uint32 UniqueID = 0;

        friend std::strong_ordering operator<=>(HotfixId const& left, HotfixId const& right) = default;
    };

    struct HotfixRecord
    {
        enum class Status : uint8
        {
            NotSet          = 0,
            Valid           = 1,
            RecordRemoved   = 2,
            Invalid         = 3,
            NotPublic       = 4
        };

        uint32 TableHash = 0;
        int32 RecordID = 0;
        HotfixId ID;
        Status HotfixStatus = Status::Invalid;

        uint32 AvailableLocalesMask = 0;

        friend std::strong_ordering operator<=>(HotfixRecord const& left, HotfixRecord const& right)
        {
            if (std::strong_ordering cmp = left.ID <=> right.ID; cmp != 0)
                return cmp;
            if (std::strong_ordering cmp = left.TableHash <=> right.TableHash; cmp != 0)
                return cmp;
            if (std::strong_ordering cmp = left.RecordID <=> right.RecordID; cmp != 0)
                return cmp;
            return std::strong_ordering::equal;
        }
    };

    struct HotfixOptionalData
    {
        uint32 Key = 0;
        std::vector<uint8> Data;
    };

    struct HotfixPush
    {
        std::vector<HotfixRecord> Records;
        uint32 AvailableLocalesMask = 0;
    };

    using HotfixContainer = std::map<int32, HotfixPush>;

    static DB2Manager& Instance();

    // Registers a loaded store under its table hash so GetStorage() / the hotfix handlers can find it.
    void AddDB2(uint32 tableHash, DB2StorageBase* store);
    DB2StorageBase const* GetStorage(uint32 type) const;

    void LoadHotfixData(uint32 localeMask);
    void LoadHotfixBlob(uint32 localeMask);
    void LoadHotfixOptionalData(uint32 localeMask);
    uint32 GetHotfixCount() const;
    HotfixContainer const& GetHotfixData() const;
    std::vector<uint8> const* GetHotfixBlobData(uint32 tableHash, int32 recordId, LocaleConstant locale) const;
    std::vector<HotfixOptionalData> const* GetHotfixOptionalData(uint32 tableHash, int32 recordId, LocaleConstant locale) const;

    // Builds the ChrCustomization choice/display-info indexes after the DB2 stores are loaded.
    // Must be called once, after LoadDB2Stores() has populated the Choice/DisplayInfo/Element stores.
    void LoadChrCustomizationData();

    // ChrCustomizationOption.ID -> the choices that belong to that option (e.g. druid shapeshift "forms").
    std::vector<ChrCustomizationChoiceEntry const*> const* GetCustomizationChoices(uint32 chrCustomizationOptionId) const;

    // ChrCustomizationChoice.ID -> its display info (ShapeshiftFormID + DisplayID + barbershop camera data),
    // resolved through ChrCustomizationElement. This is the choice->display/model link shapeshift/barbershop need.
    ChrCustomizationDisplayInfoEntry const* GetCustomizationDisplayInfo(uint32 chrCustomizationChoiceId) const;

private:
    DB2Manager() = default;
    ~DB2Manager() = default;
    DB2Manager(DB2Manager const&) = delete;
    DB2Manager& operator=(DB2Manager const&) = delete;

    std::map<uint32 /*tableHash*/, DB2StorageBase*> _stores;
    HotfixContainer _hotfixData;
    std::array<std::map<std::pair<uint32 /*tableHash*/, int32 /*recordId*/>, std::vector<uint8>>, TOTAL_LOCALES> _hotfixBlob;
    std::array<std::map<std::pair<uint32 /*tableHash*/, int32 /*recordId*/>, std::vector<HotfixOptionalData>>, TOTAL_LOCALES> _hotfixOptionalData;
    int32 _maxHotfixId = 0;

    std::unordered_map<uint32 /*chrCustomizationOptionId*/, std::vector<ChrCustomizationChoiceEntry const*>> _chrCustomizationChoicesByOption;
    std::unordered_map<uint32 /*chrCustomizationChoiceId*/, ChrCustomizationDisplayInfoEntry const*> _displayInfoByCustomizationChoice;
};

#define sDB2Manager DB2Manager::Instance()

// Loads the DB2 store subset from <dataPath>/dbc/<locale>/ at worldserver boot.
void LoadDB2Stores(std::string const& dataPath, LocaleConstant defaultLocale);

#endif // AC_DB2STORES_H
