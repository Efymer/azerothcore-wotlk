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

// World-entry DB2 store subset for build 3.4.3.54261 (Phase 1c). DB2-only stores (no DBC twin)
// live here now; DBC-colliding stores are added during the DBC->DB2 repoint (Task 1c.3).
extern DB2Storage<ChrClassesEntry> sChrClassesStore;            // migrated from DBC (1c.3)
extern DB2Storage<ChrRacesEntry> sChrRacesStore;                // migrated from DBC (1c.3)
extern DB2Storage<SkillLineEntry> sSkillLineStore;             // migrated from DBC (1c.3)
extern DB2Storage<SkillLineAbilityEntry> sSkillLineAbilityStore; // migrated from DBC (1c.3)
extern DB2Storage<LiquidMaterialEntry> sLiquidMaterialStore;
extern DB2Storage<SpellNameEntry> sSpellNameStore;
extern DB2Storage<CharacterLoadoutEntry> sCharacterLoadoutStore;
extern DB2Storage<CharacterLoadoutItemEntry> sCharacterLoadoutItemStore;
extern DB2Storage<ChrCustomizationOptionEntry> sChrCustomizationOptionStore;
extern DB2Storage<ChrCustomizationReqEntry> sChrCustomizationReqStore;
extern DB2Storage<ItemEffectEntry> sItemEffectStore;
extern DB2Storage<ItemAppearanceEntry> sItemAppearanceStore;
extern DB2Storage<ItemModifiedAppearanceEntry> sItemModifiedAppearanceStore;
extern DB2Storage<PowerTypeEntry> sPowerTypeStore;
extern DB2Storage<ItemSparseEntry> sItemSparseStore;
extern DB2Storage<PlayerConditionEntry> sPlayerConditionStore;
extern DB2Storage<PowerDisplayEntry> sPowerDisplayStore;        // migrated from DBC (1c.3)

// Loads the DB2 store subset from <dataPath>/dbc/<locale>/ at worldserver boot.
void LoadDB2Stores(std::string const& dataPath, LocaleConstant defaultLocale);

#endif // AC_DB2STORES_H
