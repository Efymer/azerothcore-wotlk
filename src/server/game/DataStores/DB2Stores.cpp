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
#include "Containers.h"
#include "DatabaseEnv.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "Field.h"
#include "Log.h"
#include "QueryResult.h"
#include "StringFormat.h"
#include "Timer.h"
#include <bitset>
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
DB2Storage<ChrCustomizationChoiceEntry> sChrCustomizationChoiceStore("ChrCustomizationChoice.db2", &ChrCustomizationChoiceLoadInfo::Instance);
DB2Storage<ChrCustomizationDisplayInfoEntry> sChrCustomizationDisplayInfoStore("ChrCustomizationDisplayInfo.db2", &ChrCustomizationDisplayInfoLoadInfo::Instance);
DB2Storage<ChrCustomizationElementEntry> sChrCustomizationElementStore("ChrCustomizationElement.db2", &ChrCustomizationElementLoadInfo::Instance);
DB2Storage<ItemEffectEntry>             sItemEffectStore("ItemEffect.db2", &ItemEffectLoadInfo::Instance);
DB2Storage<ItemAppearanceEntry>         sItemAppearanceStore("ItemAppearance.db2", &ItemAppearanceLoadInfo::Instance);
DB2Storage<ItemModifiedAppearanceEntry> sItemModifiedAppearanceStore("ItemModifiedAppearance.db2", &ItemModifiedAppearanceLoadInfo::Instance);
DB2Storage<PowerTypeEntry>              sPowerTypeStore("PowerType.db2", &PowerTypeLoadInfo::Instance);
DB2Storage<ItemEntry>                   sItemStore("Item.db2", &ItemLoadInfo::Instance);
DB2Storage<ItemSparseEntry>             sItemSparseStore("ItemSparse.db2", &ItemSparseLoadInfo::Instance);
DB2Storage<PlayerConditionEntry>        sPlayerConditionStore("PlayerCondition.db2", &PlayerConditionLoadInfo::Instance);
DB2Storage<PowerDisplayEntry>           sPowerDisplayStore("PowerDisplay.db2", &PowerDisplayLoadInfo::Instance);
DB2Storage<TaxiNodesEntry>              sTaxiNodesStore("TaxiNodes.db2", &TaxiNodesLoadInfo::Instance);
DB2Storage<TaxiPathEntry>               sTaxiPathStore("TaxiPath.db2", &TaxiPathLoadInfo::Instance);
DB2Storage<TaxiPathNodeEntry>           sTaxiPathNodeStore("TaxiPathNode.db2", &TaxiPathNodeLoadInfo::Instance);
DB2Storage<AreaTableEntry>              sAreaTableStore("AreaTable.db2", &AreaTableLoadInfo::Instance);
DB2Storage<FactionEntry>                sFactionStore("Faction.db2", &FactionLoadInfo::Instance);
DB2Storage<FactionTemplateEntry>        sFactionTemplateStore("FactionTemplate.db2", &FactionTemplateLoadInfo::Instance);
DB2Storage<MapEntry>                    sMapStore("Map.db2", &MapLoadInfo::Instance);
DB2Storage<MapDifficultyEntry>          sMapDifficultyStore("MapDifficulty.db2", &MapDifficultyLoadInfo::Instance);
DB2Storage<CinematicCameraEntry>        sCinematicCameraStore("CinematicCamera.db2", &CinematicCameraLoadInfo::Instance);
DB2Storage<CinematicSequencesEntry>     sCinematicSequencesStore("CinematicSequences.db2", &CinematicSequencesLoadInfo::Instance);
DB2Storage<CurrencyTypesEntry>          sCurrencyTypesStore("CurrencyTypes.db2", &CurrencyTypesLoadInfo::Instance);

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
            sDB2Manager.AddDB2((store).GetTableHash(), &(store)); \
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
    LOAD_DB2(sChrCustomizationChoiceStore);
    LOAD_DB2(sChrCustomizationDisplayInfoStore);
    LOAD_DB2(sChrCustomizationElementStore);
    LOAD_DB2(sItemEffectStore);
    LOAD_DB2(sItemAppearanceStore);
    LOAD_DB2(sItemModifiedAppearanceStore);
    LOAD_DB2(sPowerTypeStore);
    LOAD_DB2(sItemStore);
    LOAD_DB2(sItemSparseStore);
    LOAD_DB2(sPlayerConditionStore);
    LOAD_DB2(sPowerDisplayStore);
    LOAD_DB2(sTaxiNodesStore);
    LOAD_DB2(sTaxiPathStore);
    LOAD_DB2(sTaxiPathNodeStore);
    LOAD_DB2(sAreaTableStore);
    LOAD_DB2(sFactionStore);
    LOAD_DB2(sFactionTemplateStore);
    LOAD_DB2(sMapStore);
    LOAD_DB2(sMapDifficultyStore);
    LOAD_DB2(sCinematicCameraStore);
    LOAD_DB2(sCinematicSequencesStore);
    LOAD_DB2(sCurrencyTypesStore);

#undef LOAD_DB2

    for (std::string const& error : loadErrors)
        LOG_ERROR("server.loading", "Could not load DB2 store: {}", error);

    sDB2Manager.LoadChrCustomizationData();

    LOG_INFO("server.loading", ">> Initialized {} DB2 data stores in {} ms", loadedStores, GetMSTimeDiffToNow(oldMSTime));
}

DB2Manager& DB2Manager::Instance()
{
    static DB2Manager instance;
    return instance;
}

void DB2Manager::AddDB2(uint32 tableHash, DB2StorageBase* store)
{
    _stores[tableHash] = store;
}

DB2StorageBase const* DB2Manager::GetStorage(uint32 type) const
{
    auto itr = _stores.find(type);
    if (itr != _stores.end())
        return itr->second;

    return nullptr;
}

void DB2Manager::LoadChrCustomizationData()
{
    _chrCustomizationChoicesByOption.clear();
    _displayInfoByCustomizationChoice.clear();

    for (ChrCustomizationChoiceEntry const* customizationChoice : sChrCustomizationChoiceStore)
        _chrCustomizationChoicesByOption[customizationChoice->ChrCustomizationOptionID].push_back(customizationChoice);

    // ChrCustomizationElement links a customization choice to the display info (shapeshift form / model)
    // it produces. Resolve choice -> displayInfo so shapeshift and barbershop code can look it up directly.
    for (ChrCustomizationElementEntry const* customizationElement : sChrCustomizationElementStore)
    {
        if (ChrCustomizationDisplayInfoEntry const* customizationDisplayInfo = sChrCustomizationDisplayInfoStore.LookupEntry(customizationElement->ChrCustomizationDisplayInfoID))
            if (sChrCustomizationChoiceStore.LookupEntry(customizationElement->ChrCustomizationChoiceID))
                _displayInfoByCustomizationChoice[customizationElement->ChrCustomizationChoiceID] = customizationDisplayInfo;
    }

    // [1c.4] TODO: the full per-(race, gender, form) ShapeshiftFormModelData index and
    // GetShapeshiftFormModelData()/GetCustomizationOptions(race, gender) accessors still need the
    // ChrModel, ChrRaceXChrModel and ChrCustomizationReqChoice DB2 stores (not yet ported to AC).
    // The choice->displayInfo and option->choices maps above are the pieces resolvable from the
    // currently-loaded stores; wiring GetModelForShapeshift can build on them.

    LOG_INFO("server.loading", ">> Indexed {} ChrCustomization options and {} choice display infos",
        _chrCustomizationChoicesByOption.size(), _displayInfoByCustomizationChoice.size());
}

std::vector<ChrCustomizationChoiceEntry const*> const* DB2Manager::GetCustomizationChoices(uint32 chrCustomizationOptionId) const
{
    return Acore::Containers::MapGetValuePtr(_chrCustomizationChoicesByOption, chrCustomizationOptionId);
}

ChrCustomizationDisplayInfoEntry const* DB2Manager::GetCustomizationDisplayInfo(uint32 chrCustomizationChoiceId) const
{
    // The map stores pointers, so MapGetValuePtr returns the stored pointer directly (not a pointer-to-pointer).
    return Acore::Containers::MapGetValuePtr(_displayInfoByCustomizationChoice, chrCustomizationChoiceId);
}

void DB2Manager::LoadHotfixData(uint32 localeMask)
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = HotfixDatabase.Query("SELECT Id, UniqueId, TableHash, RecordId, Status FROM hotfix_data ORDER BY Id");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 hotfix info entries.");
        return;
    }

    uint32 count = 0;

    std::map<std::pair<uint32, int32>, bool> deletedRecords;

    do
    {
        Field* fields = result->Fetch();

        int32 id = fields[0].Get<int32>();
        uint32 uniqueId = fields[1].Get<uint32>();
        uint32 tableHash = fields[2].Get<uint32>();
        int32 recordId = fields[3].Get<int32>();
        HotfixRecord::Status status = static_cast<HotfixRecord::Status>(fields[4].Get<uint8>());
        std::bitset<TOTAL_LOCALES> availableDb2Locales = localeMask;
        if (status == HotfixRecord::Status::Valid && !_stores.contains(tableHash))
        {
            std::pair<uint32, int32> key = std::make_pair(tableHash, recordId);
            for (std::size_t locale = 0; locale < TOTAL_LOCALES; ++locale)
            {
                if (!availableDb2Locales[locale])
                    continue;

                if (!_hotfixBlob[locale].contains(key))
                    availableDb2Locales[locale] = false;
            }

            if (availableDb2Locales.none())
            {
                LOG_ERROR("sql.sql", "Table `hotfix_data` references unknown DB2 store by hash 0x{:X} and has no reference to `hotfix_blob` in hotfix id {} with RecordID: {}", tableHash, id, recordId);
                continue;
            }
        }

        HotfixRecord hotfixRecord;
        hotfixRecord.TableHash = tableHash;
        hotfixRecord.RecordID = recordId;
        hotfixRecord.ID.PushID = id;
        hotfixRecord.ID.UniqueID = uniqueId;
        hotfixRecord.HotfixStatus = status;
        hotfixRecord.AvailableLocalesMask = availableDb2Locales.to_ulong();

        HotfixPush& push = _hotfixData[id];
        push.Records.push_back(hotfixRecord);
        push.AvailableLocalesMask |= hotfixRecord.AvailableLocalesMask;

        _maxHotfixId = std::max(_maxHotfixId, id);
        deletedRecords[std::make_pair(tableHash, recordId)] = status == HotfixRecord::Status::RecordRemoved;
        ++count;
    } while (result->NextRow());

    for (auto itr = deletedRecords.begin(); itr != deletedRecords.end(); ++itr)
        if (itr->second)
            if (DB2StorageBase* store = Acore::Containers::MapGetValuePtr(_stores, itr->first.first))
                store->EraseRecord(itr->first.second);

    LOG_INFO("server.loading", ">> Loaded {} hotfix records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void DB2Manager::LoadHotfixBlob(uint32 localeMask)
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = HotfixDatabase.Query("SELECT TableHash, RecordId, locale, `Blob` FROM hotfix_blob ORDER BY TableHash");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 hotfix blob entries.");
        return;
    }

    std::bitset<TOTAL_LOCALES> availableDb2Locales = localeMask;
    uint32 hotfixBlobCount = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 tableHash = fields[0].Get<uint32>();
        auto storeItr = _stores.find(tableHash);
        if (storeItr != _stores.end())
        {
            LOG_ERROR("sql.sql", "Table hash 0x{:X} points to a loaded DB2 store {}, fill related table instead of hotfix_blob",
                tableHash, storeItr->second->GetFileName());
            continue;
        }

        int32 recordId = fields[1].Get<int32>();
        std::string localeName = fields[2].Get<std::string>();
        LocaleConstant locale = GetLocaleByName(localeName);

        if (locale >= TOTAL_LOCALES)
        {
            LOG_ERROR("sql.sql", "`hotfix_blob` contains invalid locale: {} at TableHash: 0x{:X} and RecordID: {}", localeName, tableHash, recordId);
            continue;
        }

        if (!availableDb2Locales[locale])
            continue;

        _hotfixBlob[locale][std::make_pair(tableHash, recordId)] = fields[3].Get<Binary>();
        hotfixBlobCount++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} hotfix blob records in {} ms", hotfixBlobCount, GetMSTimeDiffToNow(oldMSTime));
}

void DB2Manager::LoadHotfixOptionalData(uint32 localeMask)
{
    // NOTE: TrinityCore registers an allow-list of (DB2 store, optional-data key, validator) tuples here
    // (e.g. BroadcastText -> TactKey). AzerothCore's 3.4.3 DB2 subset has neither store, and
    // acore_hotfixes.hotfix_optional_data is empty, so the allow-list is intentionally omitted. The query is
    // kept verbatim; with 0 rows it returns early. If optional-data support is ever needed, restore the
    // allow-list registration (see reference DB2Stores.cpp::LoadHotfixOptionalData).
    uint32 oldMSTime = getMSTime();

    QueryResult result = HotfixDatabase.Query("SELECT TableHash, RecordId, locale, `Key`, `Data` FROM hotfix_optional_data ORDER BY TableHash");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 hotfix optional data records.");
        return;
    }

    std::bitset<TOTAL_LOCALES> availableDb2Locales = localeMask;
    uint32 hotfixOptionalDataCount = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 tableHash = fields[0].Get<uint32>();
        uint32 recordId = fields[1].Get<int32>();
        auto storeItr = _stores.find(tableHash);
        if (storeItr == _stores.end())
        {
            LOG_ERROR("sql.sql", "Table `hotfix_optional_data` references unknown DB2 store by hash 0x{:X} with RecordID: {}", tableHash, recordId);
            continue;
        }

        std::string localeName = fields[2].Get<std::string>();
        LocaleConstant locale = GetLocaleByName(localeName);

        if (locale >= TOTAL_LOCALES)
        {
            LOG_ERROR("sql.sql", "`hotfix_optional_data` contains invalid locale: {} at TableHash: 0x{:X} and RecordID: {}", localeName, tableHash, recordId);
            continue;
        }

        if (!availableDb2Locales[locale])
            continue;

        DB2Manager::HotfixOptionalData optionalData;
        optionalData.Key = fields[3].Get<uint32>();
        optionalData.Data = fields[4].Get<Binary>();
        _hotfixOptionalData[locale][std::make_pair(tableHash, recordId)].push_back(std::move(optionalData));
        hotfixOptionalDataCount++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} hotfix optional data records in {} ms", hotfixOptionalDataCount, GetMSTimeDiffToNow(oldMSTime));
}

uint32 DB2Manager::GetHotfixCount() const
{
    return _hotfixData.size();
}

DB2Manager::HotfixContainer const& DB2Manager::GetHotfixData() const
{
    return _hotfixData;
}

std::vector<uint8> const* DB2Manager::GetHotfixBlobData(uint32 tableHash, int32 recordId, LocaleConstant locale) const
{
    ASSERT(locale < TOTAL_LOCALES, "Locale {} is invalid locale", uint32(locale));

    return Acore::Containers::MapGetValuePtr(_hotfixBlob[locale], std::make_pair(tableHash, recordId));
}

std::vector<DB2Manager::HotfixOptionalData> const* DB2Manager::GetHotfixOptionalData(uint32 tableHash, int32 recordId, LocaleConstant locale) const
{
    ASSERT(locale < TOTAL_LOCALES, "Locale {} is invalid locale", uint32(locale));

    return Acore::Containers::MapGetValuePtr(_hotfixOptionalData[locale], std::make_pair(tableHash, recordId));
}
