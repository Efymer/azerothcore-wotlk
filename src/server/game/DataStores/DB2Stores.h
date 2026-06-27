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

// World-entry DB2 store subset for build 3.4.3.54261 (Phase 1c).
// More stores are added here as the world-entry code paths are switched from DBC to DB2.
extern DB2Storage<LiquidMaterialEntry> sLiquidMaterialStore;

// Loads the DB2 store subset from <dataPath>/dbc/<locale>/ at worldserver boot.
void LoadDB2Stores(std::string const& dataPath, LocaleConstant defaultLocale);

#endif // AC_DB2STORES_H
