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

#include "Corpse.h"
#include "CharacterCache.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "UpdateMask.h"
#include "World.h"

Corpse::Corpse(CorpseType type) : WorldObject(), m_type(type)
{
    m_objectType |= TYPEMASK_CORPSE;
    m_objectTypeId = TYPEID_CORPSE;
    m_updateFlag.Stationary = true;
    m_time = GameTime::GetGameTime().count();
    lootRecipient = nullptr;
}

Corpse::~Corpse()
{
}

void Corpse::AddToWorld()
{
    ///- Register the corpse for guid lookup
    if (!IsInWorld())
        GetMap()->GetObjectsStore().Insert<Corpse>(GetGUID(), this);

    Object::AddToWorld();
}

void Corpse::RemoveFromWorld()
{
    ///- Remove the corpse from the accessor
    if (IsInWorld())
        GetMap()->GetObjectsStore().Remove<Corpse>(GetGUID());

    WorldObject::RemoveFromWorld();
}

bool Corpse::Create(ObjectGuid::LowType guidlow)
{
    Object::_Create(guidlow, 0, HighGuid::Corpse);
    return true;
}

bool Corpse::Create(ObjectGuid::LowType guidlow, Player* owner)
{
    ASSERT(owner);

    Relocate(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(), owner->GetOrientation());

    if (!IsPositionValid())
    {
        LOG_ERROR("entities.player", "Corpse (guidlow {}, owner {}) not created. Suggested coordinates isn't valid (X: {} Y: {})",
            guidlow, owner->GetName(), owner->GetPositionX(), owner->GetPositionY());
        return false;
    }

    WorldObject::_Create(guidlow, HighGuid::Corpse, owner->GetPhaseMask());

    SetObjectScale(1);
    SetUpdateFieldValue(m_values.ModifyValue(&Corpse::m_corpseData).ModifyValue(&UF::CorpseData::Owner), owner->GetGUID());

    _cellCoord = Acore::ComputeCellCoord(GetPositionX(), GetPositionY());

    return true;
}

void Corpse::SaveToDB()
{
    // prevent DB data inconsistence problems and duplicates
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    DeleteFromDB(trans);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CORPSE);
    stmt->SetData(0, GetOwnerGUID().GetCounter());                            // guid
    stmt->SetData (1, GetPositionX());                                         // posX
    stmt->SetData (2, GetPositionY());                                         // posY
    stmt->SetData (3, GetPositionZ());                                         // posZ
    stmt->SetData (4, GetOrientation());                                       // orientation
    stmt->SetData(5, GetMapId());                                             // mapId
    stmt->SetData(6, uint32(m_corpseData->DisplayID));                        // displayId
    // itemCache: serialize the 19-slot Items array as the legacy space-separated field string
    {
        std::ostringstream items;
        for (std::size_t i = 0; i < m_corpseData->Items.size(); ++i)
            items << uint32(m_corpseData->Items[i]) << ' ';
        stmt->SetData(7, items.str());                                       // itemCache
    }
    // Legacy bytes1 layout: byte1=race, byte2=gender; appearance (skin/face/hair) is genuinely-absent
    // in the structured CorpseData (now ChrCustomizationChoice array) -> see [1c.4] TODO below.
    stmt->SetData(8, uint32((uint32(m_corpseData->RaceID) << 8) | (uint32(m_corpseData->Sex) << 16))); // bytes1
    stmt->SetData(9, uint32(0));                                              // bytes2 // [1c.4] TODO: appearance customizations
    stmt->SetData(10, uint32(m_corpseData->GuildGUID->GetCounter()));         // guildId
    stmt->SetData (11, uint32(m_corpseData->Flags));                           // flags
    stmt->SetData (12, uint32(m_corpseData->DynamicFlags));                    // dynFlags
    stmt->SetData(13, uint32(m_time));                                        // time
    stmt->SetData (14, GetType());                                             // corpseType
    stmt->SetData(15, GetInstanceId());                                       // instanceId
    stmt->SetData(16, GetPhaseMask());                                        // phaseMask
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void Corpse::DeleteFromDB(CharacterDatabaseTransaction trans)
{
    DeleteFromDB(GetOwnerGUID(), trans);
}

void Corpse::DeleteFromDB(ObjectGuid const& ownerGuid, CharacterDatabaseTransaction trans)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CORPSE);
    stmt->SetData(0, ownerGuid.GetCounter());
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

bool Corpse::LoadCorpseFromDB(ObjectGuid::LowType guid, Field* fields)
{
    ObjectGuid::LowType ownerGuid = fields[16].Get<uint32>();

    //        0     1     2     3            4      5          6          7       8       9        10     11        12    13          14          15         16
    // SELECT posX, posY, posZ, orientation, mapId, displayId, itemCache, bytes1, bytes2, guildId, flags, dynFlags, time, corpseType, instanceId, phaseMask, guid FROM corpse WHERE mapId = ? AND instanceId = ?
    float posX   = fields[0].Get<float>();
    float posY   = fields[1].Get<float>();
    float posZ   = fields[2].Get<float>();
    float o      = fields[3].Get<float>();
    uint32 mapId = fields[4].Get<uint16>();

    Object::_Create(guid, 0, HighGuid::Corpse);

    SetObjectScale(1.0f);
    auto corpseData = m_values.ModifyValue(&Corpse::m_corpseData);
    SetUpdateFieldValue(corpseData.ModifyValue(&UF::CorpseData::DisplayID), fields[5].Get<uint32>());

    // itemCache: parse the legacy space-separated field string back into the Items array
    {
        std::string const itemCache = fields[6].Get<std::string>();
        std::istringstream iss(itemCache);
        uint32 itemValue;
        for (uint32 i = 0; i < EQUIPMENT_SLOT_END && (iss >> itemValue); ++i)
            SetUpdateFieldValue(m_values.ModifyValue(&Corpse::m_corpseData).ModifyValue(&UF::CorpseData::Items, i), itemValue);
    }

    // Legacy bytes1 -> RaceID (byte1) + Sex (byte2); bytes2 held appearance (skin/face/hair) which is
    // now ChrCustomizationChoice data and is genuinely-absent here. [1c.4] TODO: rebuild Customizations.
    uint32 const bytes1 = fields[7].Get<uint32>();
    SetUpdateFieldValue(corpseData.ModifyValue(&UF::CorpseData::RaceID), uint8(bytes1 >> 8));
    SetUpdateFieldValue(corpseData.ModifyValue(&UF::CorpseData::Sex), uint8(bytes1 >> 16));
    // fields[8] (bytes2) -> [1c.4] TODO: appearance customizations
    // fields[9] (guildId) -> [1c.4] TODO: CorpseData::GuildGUID is an ObjectGuid but AC 3.3.5 has no
    // HighGuid::Guild; guild-guid mapping is KNOWN-absent, left empty.
    SetUpdateFieldValue(corpseData.ModifyValue(&UF::CorpseData::Flags), uint32(fields[10].Get<uint8>()));
    SetUpdateFieldValue(corpseData.ModifyValue(&UF::CorpseData::DynamicFlags), uint32(fields[11].Get<uint8>()));
    SetUpdateFieldValue(corpseData.ModifyValue(&UF::CorpseData::Owner), ObjectGuid::Create<HighGuid::Player>(ownerGuid));

    m_time = time_t(fields[12].Get<uint32>());

    uint32 instanceId  = fields[14].Get<uint32>();
    uint32 phaseMask   = fields[15].Get<uint32>();

    // place
    SetLocationInstanceId(instanceId);
    SetLocationMapId(mapId);
    SetPhaseMask(phaseMask, false);
    Relocate(posX, posY, posZ, o);

    if (!IsPositionValid())
    {
        LOG_ERROR("entities.player", "Corpse ( {}, owner: {}) is not created, given coordinates are not valid (X: {}, Y: {}, Z: {})",
            GetGUID().ToString(), GetOwnerGUID().ToString(), posX, posY, posZ);
        return false;
    }

    _cellCoord = Acore::ComputeCellCoord(GetPositionX(), GetPositionY());
    return true;
}

bool Corpse::IsExpired(time_t t) const
{
    // Deleted character
    if (!sCharacterCache->GetCharacterCacheByGuid(GetOwnerGUID()))
        return true;

    if (m_type == CORPSE_BONES)
        return m_time < t - 60 * MINUTE;
    else
        return m_time < t - 3 * DAY;
}

void Corpse::ResetGhostTime()
{
    m_time = GameTime::GetGameTime().count();
}

// Legacy 3.3.5 Corpse::BuildValuesUpdate(uint8 updateType, ...) removed in the 54261 structured-UF cutover.
// Two-sided-raid corpse appearance masking (CORPSE_FIELD_BYTES_1/2) must be reimplemented via
// ViewerDependentValues for CorpseData when the call-site sweep reaches Corpse.

void Corpse::BuildValuesCreate(ByteBuffer* data, Player const* target) const
{
    UF::UpdateFieldFlag flags = GetUpdateFieldFlagsFor(target);
    std::size_t sizePos = data->wpos();
    *data << uint32(0);
    *data << uint8(flags);
    m_objectData->WriteCreate(*data, flags, this, target);
    m_corpseData->WriteCreate(*data, flags, this, target);
    data->put<uint32>(sizePos, data->wpos() - sizePos - 4);
}

void Corpse::BuildValuesUpdate(ByteBuffer* data, Player const* target) const
{
    UF::UpdateFieldFlag flags = GetUpdateFieldFlagsFor(target);
    std::size_t sizePos = data->wpos();
    *data << uint32(0);
    *data << uint32(m_values.GetChangedObjectTypeMask());

    if (m_values.HasChanged(TYPEID_OBJECT))
        m_objectData->WriteUpdate(*data, flags, this, target);

    if (m_values.HasChanged(TYPEID_CORPSE))
        m_corpseData->WriteUpdate(*data, flags, this, target);

    data->put<uint32>(sizePos, data->wpos() - sizePos - 4);
}

void Corpse::ClearUpdateMask(bool remove)
{
    m_values.ClearChangesMask(&Corpse::m_corpseData);
    Object::ClearUpdateMask(remove);
}
