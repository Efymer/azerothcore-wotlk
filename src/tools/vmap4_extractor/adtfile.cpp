/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "vmapexport.h"
#include "adtfile.h"
#include "StringFormat.h"
#include "Errors.h"
#include <cstdio>
#include <cstring>

char const* GetPlainName(char const* FileName)
{
    char const* szTemp;

    if ((szTemp = strrchr(FileName, '\\')) != nullptr)
        FileName = szTemp + 1;
    return FileName;
}

char* GetPlainName(char* FileName)
{
    char * szTemp;

    if ((szTemp = strrchr(FileName, '\\')) != nullptr)
        FileName = szTemp + 1;
    return FileName;
}

void FixNameCase(char* name, size_t len)
{
    char* ptr = name + len - 1;

    //extension in lowercase
    for (; *ptr != '.'; --ptr)
        *ptr |= 0x20;

    for (; ptr >= name; --ptr)
    {
        if (ptr > name && *ptr >= 'A' && *ptr <= 'Z' && isalpha(*(ptr - 1)))
            *ptr |= 0x20;
        else if ((ptr == name || !isalpha(*(ptr - 1))) && *ptr >= 'a' && *ptr <= 'z')
            *ptr &= ~0x20;
    }
}

void FixNameSpaces(char* name, size_t len)
{
    if (len < 3)
        return;

    for (size_t i = 0; i < len - 3; i++)
        if (name[i] == ' ')
            name[i] = '_';
}

void NormalizeFileName(char* name, size_t len)
{
    if (len >= 4 && !memcmp(name, "FILE", 4)) // name is FileDataId formatted, do not normalize
        return;

    FixNameCase(name, len);
    FixNameSpaces(name, len);
}

char* GetExtension(char* FileName)
{
    if (char* szTemp = strrchr(FileName, '.'))
        return szTemp;
    return nullptr;
}

extern std::shared_ptr<CASC::Storage> CascStorage;

ADTFile::ADTFile(std::string const& filename) : _file(CascStorage, filename.c_str(), false)
{
}

ADTFile::ADTFile(uint32 fileDataId, std::string const& description) : _file(CascStorage, fileDataId, description, false)
{
}

bool ADTFile::init(uint32 map_num, uint32 tileX, uint32 tileY)
{
    if (_file.isEof())
        return false;

    uint32 size;
    std::string dirname = Acore::StringFormat("{}/dir_bin", szWorkDirWmo);
    FILE* dirfile = fopen(dirname.c_str(), "ab");
    if (!dirfile)
    {
        printf("Can't open dirfile!'%s'\n", dirname.c_str());
        return false;
    }

    while (!_file.isEof())
    {
        char fourcc[5];
        _file.read(&fourcc,4);
        _file.read(&size, 4);
        flipcc(fourcc);
        fourcc[4] = 0;

        size_t nextpos = _file.getPos() + size;

        if (!strcmp(fourcc,"MMDX"))
        {
            if (size)
            {
                char* buf = new char[size];
                _file.read(buf, size);
                char* p = buf;
                while (p < buf + size)
                {
                    std::string path(p);

                    char* s = GetPlainName(p);
                    NormalizeFileName(s, strlen(s));

                    ModelInstanceNames.emplace_back(s);

                    ExtractSingleModel(path);

                    p += strlen(p) + 1;
                }
                delete[] buf;
            }
        }
        else if (!strcmp(fourcc,"MWMO"))
        {
            if (size)
            {
                char* buf = new char[size];
                _file.read(buf, size);
                char* p = buf;
                while (p < buf + size)
                {
                    std::string path(p);

                    char* s = GetPlainName(p);
                    NormalizeFileName(s, strlen(s));

                    WmoInstanceNames.emplace_back(s);

                    ExtractSingleWmo(path);

                    p += strlen(p) + 1;
                }
                delete[] buf;
            }
        }
        //======================
        else if (!strcmp(fourcc, "MDDF"))
        {
            if (size)
            {
                uint32 doodadCount = size / sizeof(ADT::MDDF);
                for (uint32 i = 0; i < doodadCount; ++i)
                {
                    ADT::MDDF doodadDef;
                    _file.read(&doodadDef, sizeof(ADT::MDDF));
                    if (!(doodadDef.Flags & 0x40))
                    {
                        Doodad::Extract(doodadDef, ModelInstanceNames[doodadDef.Id].c_str(), map_num, tileX, tileY, dirfile);
                    }
                    else
                    {
                        std::string fileName = Acore::StringFormat("FILE{:08X}.xxx", doodadDef.Id);
                        ExtractSingleModel(fileName);
                        Doodad::Extract(doodadDef, fileName.c_str(), map_num, tileX, tileY, dirfile);
                    }
                }

                ModelInstanceNames.clear();
            }
        }
        else if (!strcmp(fourcc,"MODF"))
        {
            if (size)
            {
                uint32 mapObjectCount = size / sizeof(ADT::MODF);
                for (uint32 i = 0; i < mapObjectCount; ++i)
                {
                    ADT::MODF mapObjDef;
                    _file.read(&mapObjDef, sizeof(ADT::MODF));
                    if (!(mapObjDef.Flags & 0x8))
                    {
                        MapObject::Extract(mapObjDef, WmoInstanceNames[mapObjDef.Id].c_str(), map_num, tileX, tileY, dirfile);
                        Doodad::ExtractSet(WmoDoodads[WmoInstanceNames[mapObjDef.Id]], mapObjDef, map_num, tileX, tileY, dirfile);
                    }
                    else
                    {
                        std::string fileName = Acore::StringFormat("FILE{:08X}.xxx", mapObjDef.Id);
                        ExtractSingleWmo(fileName);
                        MapObject::Extract(mapObjDef, fileName.c_str(), map_num, tileX, tileY, dirfile);
                        Doodad::ExtractSet(WmoDoodads[fileName], mapObjDef, map_num, tileX, tileY, dirfile);
                    }
                }

                WmoInstanceNames.clear();
            }
        }

        //======================
        _file.seek(nextpos);
    }

    _file.close();
    fclose(dirfile);
    return true;
}

ADTFile::~ADTFile()
{
    _file.close();
}
