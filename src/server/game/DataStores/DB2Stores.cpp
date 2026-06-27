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

#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "Log.h"
#include "StringFormat.h"
#include "Timer.h"
#include <vector>

DB2Storage<ChrClassesEntry>             sChrClassesStore("ChrClasses.db2", &ChrClassesLoadInfo::Instance);
DB2Storage<ChrRacesEntry>               sChrRacesStore("ChrRaces.db2", &ChrRacesLoadInfo::Instance);
DB2Storage<SkillLineEntry>              sSkillLineStore("SkillLine.db2", &SkillLineLoadInfo::Instance);
DB2Storage<SkillLineAbilityEntry>       sSkillLineAbilityStore("SkillLineAbility.db2", &SkillLineAbilityLoadInfo::Instance);
DB2Storage<LightEntry>                  sLightStore("Light.db2", &LightLoadInfo::Instance);
DB2Storage<WorldMapOverlayEntry>        sWorldMapOverlayStore("WorldMapOverlay.db2", &WorldMapOverlayLoadInfo::Instance);
DB2Storage<SkillRaceClassInfoEntry>     sSkillRaceClassInfoStore("SkillRaceClassInfo.db2", &SkillRaceClassInfoLoadInfo::Instance);
DB2Storage<CharTitlesEntry>             sCharTitlesStore("CharTitles.db2", &CharTitlesLoadInfo::Instance);
DB2Storage<LiquidMaterialEntry>         sLiquidMaterialStore("LiquidMaterial.db2", &LiquidMaterialLoadInfo::Instance);
DB2Storage<SpellNameEntry>              sSpellNameStore("SpellName.db2", &SpellNameLoadInfo::Instance);
DB2Storage<CharacterLoadoutEntry>       sCharacterLoadoutStore("CharacterLoadout.db2", &CharacterLoadoutLoadInfo::Instance);
DB2Storage<CharacterLoadoutItemEntry>   sCharacterLoadoutItemStore("CharacterLoadoutItem.db2", &CharacterLoadoutItemLoadInfo::Instance);
DB2Storage<ChrCustomizationOptionEntry> sChrCustomizationOptionStore("ChrCustomizationOption.db2", &ChrCustomizationOptionLoadInfo::Instance);
DB2Storage<ChrCustomizationReqEntry>    sChrCustomizationReqStore("ChrCustomizationReq.db2", &ChrCustomizationReqLoadInfo::Instance);
DB2Storage<ItemEffectEntry>             sItemEffectStore("ItemEffect.db2", &ItemEffectLoadInfo::Instance);
DB2Storage<ItemAppearanceEntry>         sItemAppearanceStore("ItemAppearance.db2", &ItemAppearanceLoadInfo::Instance);
DB2Storage<ItemModifiedAppearanceEntry> sItemModifiedAppearanceStore("ItemModifiedAppearance.db2", &ItemModifiedAppearanceLoadInfo::Instance);
DB2Storage<PowerTypeEntry>              sPowerTypeStore("PowerType.db2", &PowerTypeLoadInfo::Instance);
DB2Storage<ItemSparseEntry>             sItemSparseStore("ItemSparse.db2", &ItemSparseLoadInfo::Instance);
DB2Storage<PlayerConditionEntry>        sPlayerConditionStore("PlayerCondition.db2", &PlayerConditionLoadInfo::Instance);
DB2Storage<PowerDisplayEntry>           sPowerDisplayStore("PowerDisplay.db2", &PowerDisplayLoadInfo::Instance);
DB2Storage<TaxiNodesEntry>              sTaxiNodesStore("TaxiNodes.db2", &TaxiNodesLoadInfo::Instance);
DB2Storage<TaxiPathEntry>               sTaxiPathStore("TaxiPath.db2", &TaxiPathLoadInfo::Instance);
DB2Storage<TaxiPathNodeEntry>           sTaxiPathNodeStore("TaxiPathNode.db2", &TaxiPathNodeLoadInfo::Instance);

// The DB2 loader produces packed records (stride = DB2Meta::GetRecordSize()), so a store's C++ struct must
// have the identical packed size. Mirrors TrinityCore's GetCppRecordSize() structure check.
template<typename T>
static constexpr std::size_t GetCppRecordSize(DB2Storage<T> const&) { return sizeof(T); }

void LoadDB2Stores(std::string const& dataPath, LocaleConstant defaultLocale)
{
    uint32 oldMSTime = getMSTime();

    std::string const db2Path = dataPath + "dbc/";
    std::vector<std::string> loadErrors;
    uint32 loadedStores = 0;

#define LOAD_DB2(store) \
    do \
    { \
        std::size_t const cppSize = GetCppRecordSize(store); \
        std::size_t const metaSize = (store).GetLoadInfo()->Meta->GetRecordSize(); \
        if (cppSize != metaSize) \
        { \
            loadErrors.emplace_back(Acore::StringFormat("{}: C++ struct size {} != DB2 record size {} (packing/layout mismatch)", \
                (store).GetFileName(), cppSize, metaSize)); \
            break; \
        } \
        try \
        { \
            (store).Load(db2Path + localeNames[defaultLocale] + '/', defaultLocale); \
            LOG_INFO("server.loading", ">> DB2 {} loaded {} records", (store).GetFileName(), (store).GetNumRows()); \
            ++loadedStores; \
        } \
        catch (std::exception const& e) \
        { \
            loadErrors.emplace_back(Acore::StringFormat("{}: {}", (store).GetFileName(), e.what())); \
        } \
    } while (false)

    LOAD_DB2(sChrClassesStore);
    LOAD_DB2(sChrRacesStore);
    LOAD_DB2(sSkillLineStore);
    LOAD_DB2(sSkillLineAbilityStore);
    LOAD_DB2(sLightStore);
    LOAD_DB2(sWorldMapOverlayStore);
    LOAD_DB2(sSkillRaceClassInfoStore);
    LOAD_DB2(sCharTitlesStore);
    LOAD_DB2(sLiquidMaterialStore);
    LOAD_DB2(sSpellNameStore);
    LOAD_DB2(sCharacterLoadoutStore);
    LOAD_DB2(sCharacterLoadoutItemStore);
    LOAD_DB2(sChrCustomizationOptionStore);
    LOAD_DB2(sChrCustomizationReqStore);
    LOAD_DB2(sItemEffectStore);
    LOAD_DB2(sItemAppearanceStore);
    LOAD_DB2(sItemModifiedAppearanceStore);
    LOAD_DB2(sPowerTypeStore);
    LOAD_DB2(sItemSparseStore);
    LOAD_DB2(sPlayerConditionStore);
    LOAD_DB2(sPowerDisplayStore);
    LOAD_DB2(sTaxiNodesStore);
    LOAD_DB2(sTaxiPathStore);
    LOAD_DB2(sTaxiPathNodeStore);

#undef LOAD_DB2

    for (std::string const& error : loadErrors)
        LOG_ERROR("server.loading", "Could not load DB2 store: {}", error);

    LOG_INFO("server.loading", ">> Initialized {} DB2 data stores in {} ms", loadedStores, GetMSTimeDiffToNow(oldMSTime));
}
