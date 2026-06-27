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
#include "Log.h"
#include "StringFormat.h"
#include "Timer.h"
#include <vector>

DB2Storage<LiquidMaterialEntry> sLiquidMaterialStore("LiquidMaterial.db2", &LiquidMaterialLoadInfo::Instance);

void LoadDB2Stores(std::string const& dataPath, LocaleConstant defaultLocale)
{
    uint32 oldMSTime = getMSTime();

    std::string const db2Path = dataPath + "dbc/";
    std::vector<std::string> loadErrors;
    uint32 loadedStores = 0;

#define LOAD_DB2(store) \
    do \
    { \
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

    LOAD_DB2(sLiquidMaterialStore);

#undef LOAD_DB2

    for (std::string const& error : loadErrors)
        LOG_ERROR("server.loading", "Could not load DB2 store: {}", error);

    LOG_INFO("server.loading", ">> Initialized {} DB2 data stores in {} ms", loadedStores, GetMSTimeDiffToNow(oldMSTime));
}
