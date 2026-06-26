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

#ifndef Locales_h__
#define Locales_h__

// Compatibility shim.
//
// TrinityCore exposes locale helpers from a dedicated <Locales.h>; AzerothCore
// keeps `LocaleConstant`, `localeNames[]`, `GetLocaleByName` and `TOTAL_LOCALES`
// in <Common.h>. The bnetserver port includes "Locales.h" and uses the TC-style
// `IsValidLocale`, so this header re-exports AC's definitions and adds the small
// `IsValidLocale` helper on top of them.

#include "Common.h"
#include <string_view>

// Returns true if the given locale name maps to a known LocaleConstant.
inline LocaleConstant GetLocaleByNameView(std::string_view name)
{
    for (uint32 i = 0; i < TOTAL_LOCALES; ++i)
        if (name == localeNames[i])
            return LocaleConstant(i);
    return LOCALE_enUS;
}

inline bool IsValidLocale(std::string_view name)
{
    for (uint32 i = 0; i < TOTAL_LOCALES; ++i)
        if (name == localeNames[i])
            return true;
    return false;
}

#endif // Locales_h__
