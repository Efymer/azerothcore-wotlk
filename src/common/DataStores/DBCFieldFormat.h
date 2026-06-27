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

#ifndef DBC_FIELD_FORMAT_H
#define DBC_FIELD_FORMAT_H

// DBC column format characters. Kept in its own header (separate from DBCFileLoader.h) so that
// consumers needing only the DBCFileLoader class (e.g. DBCStores.cpp) do not pull these
// FT_* enumerators, which would clash with the DB2 loader's identically-named DBCFormer enum
// (DB2FileLoader.h) when a translation unit references both DBC and DB2 stores.
enum DbcFieldFormat
{
    FT_NA = 'x',                                              //not used or unknown, 4 byte size
    FT_NA_BYTE = 'X',                                         //not used or unknown, byte
    FT_STRING = 's',                                          //char*
    FT_FLOAT = 'f',                                           //float
    FT_INT = 'i',                                             //uint32
    FT_BYTE = 'b',                                            //uint8
    FT_SORT = 'd',                                            //sorted by this field, field is not included
    FT_IND = 'n',                                             //the same, but parsed to data
    FT_LOGIC = 'l'                                           //Logical (boolean)
};

#endif
