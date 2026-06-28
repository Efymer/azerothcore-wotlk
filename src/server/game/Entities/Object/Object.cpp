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

#include "Object.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "CellImpl.h"
#include "Chat.h"
#include "DB2Stores.h"
#include "Creature.h"
#include "DynamicVisibility.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "Log.h"
#include "MapMgr.h"
#include "MiscPackets.h"
#include "MovementPacketBuilder.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvPMgr.h"
#include "Physics.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SpellAuraEffects.h"
#include "StringConvert.h"
#include "TargetedMovementGenerator.h"
#include "TemporarySummon.h"
#include "Tokenize.h"
#include "Totem.h"
#include "Transport.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"
#include "WorldPacket.h"

/// @todo: this import is not necessary for compilation and marked as unused by the IDE
//  however, for some reasons removing it would cause a damn linking issue
//  there is probably some underlying problem with imports which should properly addressed
//  see: https://github.com/azerothcore/azerothcore-wotlk/issues/9766
#include "GridNotifiersImpl.h"

constexpr float VisibilityDistances[AsUnderlyingType(VisibilityDistanceType::Max)] =
{
    DEFAULT_VISIBILITY_DISTANCE,
    VISIBILITY_DISTANCE_TINY,
    VISIBILITY_DISTANCE_SMALL,
    VISIBILITY_DISTANCE_LARGE,
    VISIBILITY_DISTANCE_GIGANTIC,
    VISIBILITY_DISTANCE_INFINITE
};

Object::Object() : m_values(this), m_PackGUID(sizeof(uint64) + 1)
{
    m_objectTypeId      = TYPEID_OBJECT;
    m_objectType        = TYPEMASK_OBJECT;
    m_updateFlag.Clear();

    m_inWorld           = false;
    m_isNewObject       = false;
    m_isDestroyedObject = false;
    m_objectUpdated     = false;

    sScriptMgr->OnConstructObject(this);
}

WorldObject::~WorldObject()
{
    sScriptMgr->OnWorldObjectDestroy(this);
}

Object::~Object()
{
    sScriptMgr->OnDestructObject(this);

    if (IsInWorld())
    {
        LOG_FATAL("entities.object", "Object::~Object - {} deleted but still in world!!", GetGUID().ToString());
        if (isType(TYPEMASK_ITEM))
            LOG_FATAL("entities.object", "Item slot {}", ((Item*)this)->GetSlot());
        ABORT();
    }

    if (m_objectUpdated)
    {
        LOG_FATAL("entities.object", "Object::~Object - {} deleted but still in update list!!", GetGUID().ToString());
        ABORT();
    }
}

void Object::_Create(ObjectGuid const& guid)
{
    m_objectUpdated = false;
    m_guid = guid;
    m_PackGUID.Set(guid);
}

void Object::AddToWorld()
{
    if (m_inWorld)
        return;

    m_inWorld = true;

    // synchronize values mirror with values array (changes will send in updatecreate opcode any way
    ASSERT(!m_objectUpdated);
    ClearUpdateMask(false);
}

void Object::RemoveFromWorld()
{
    if (!m_inWorld)
        return;

    m_inWorld = false;

    // if we remove from world then sending changes not required
    ClearUpdateMask(true);
}

void Object::BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    if (!target)
        return;

    uint8 updateType = m_isNewObject ? UPDATETYPE_CREATE_OBJECT2 : UPDATETYPE_CREATE_OBJECT;
    uint8 objectType = m_objectTypeId;
    CreateObjectBits flags = m_updateFlag;

    if (target == this)                                      // building packet for yourself
    {
        flags.ThisIsYou = true;
        flags.ActivePlayer = true;
        objectType = TYPEID_ACTIVE_PLAYER;
    }

    if (WorldObject const* worldObject = dynamic_cast<WorldObject const*>(this))
    {
        if (!flags.MovementUpdate && !worldObject->m_movementInfo.transport.guid.IsEmpty())
            flags.MovementTransport = true;

        // AzerothCore has no AnimKit data and no SmoothPhasing on WorldObject -> those CreateObjectBits stay false.
    }

    if (Unit const* unit = ToUnit())
    {
        // AzerothCore has no IsPlayingHoverAnim() -> PlayHoverAnim stays false.

        if (unit->GetVictim())
            flags.CombatVictim = true;
    }

    ByteBuffer& buf = data->GetBuffer();
    buf << uint8(updateType);
    buf << GetGUID();
    buf << uint8(objectType);

    BuildMovementUpdate(&buf, flags, target);
    BuildValuesCreate(&buf, target);
    data->AddUpdateBlock();
}

void Object::SendUpdateToPlayer(Player* player)
{
    // send create update to player
    UpdateData upd(player->GetMapId());
    WorldPacket packet;

    if (player->HaveAtClient(GetGUID()))
        BuildValuesUpdateBlockForPlayer(&upd, player);
    else
        BuildCreateUpdateBlockForPlayer(&upd, player);
    upd.BuildPacket(&packet);
    player->SendDirectMessage(&packet);
}

void Object::BuildValuesUpdateBlockForPlayer(UpdateData* data, Player const* target) const
{
    ByteBuffer& buf = PrepareValuesUpdateBuffer(data);

    BuildValuesUpdate(&buf, target);

    data->AddUpdateBlock();
}

void Object::BuildValuesUpdateBlockForPlayerWithFlag(UpdateData* data, UF::UpdateFieldFlag flags, Player const* target) const
{
    ByteBuffer& buf = PrepareValuesUpdateBuffer(data);

    BuildValuesUpdateWithFlag(&buf, flags, target);

    data->AddUpdateBlock();
}

void Object::BuildDestroyUpdateBlock(UpdateData* data) const
{
    data->AddDestroyObject(GetGUID());
}

void Object::BuildOutOfRangeUpdateBlock(UpdateData* data) const
{
    data->AddOutOfRangeGUID(GetGUID());
}

ByteBuffer& Object::PrepareValuesUpdateBuffer(UpdateData* data) const
{
    ByteBuffer& buffer = data->GetBuffer();
    buffer << uint8(UPDATETYPE_VALUES);
    buffer << GetGUID();
    return buffer;
}

void Object::DestroyForPlayer(Player* target) const
{
    ASSERT(target);

    UpdateData updateData(target->GetMapId());
    BuildDestroyUpdateBlock(&updateData);
    WorldPacket packet;
    updateData.BuildPacket(&packet);
    target->SendDirectMessage(&packet);
}

void Object::SendOutOfRangeForPlayer(Player* target) const
{
    ASSERT(target);

    UpdateData updateData(target->GetMapId());
    BuildOutOfRangeUpdateBlock(&updateData);
    WorldPacket packet;
    updateData.BuildPacket(&packet);
    target->SendDirectMessage(&packet);
}

namespace
{
    // 3.4.3 create-object transport block, serialized from AzerothCore's 3.3.5 MovementInfo::TransportInfo.
    // AC has no operator<< for TransportInfo and no vehicleId field; map prevTime -> AC's transport.time2.
    void WriteCreateObjectMovementTransport(ByteBuffer* data, MovementInfo::TransportInfo const& transport)
    {
        bool hasPrevTime = transport.time2 != 0;
        bool hasVehicleId = false;                                      // AC MovementInfo transport has no vehicleId

        *data << transport.guid;                                        // Transport GUID
        *data << float(transport.pos.GetPositionX());
        *data << float(transport.pos.GetPositionY());
        *data << float(transport.pos.GetPositionZ());
        *data << float(transport.pos.GetOrientation());
        *data << int8(transport.seat);                                  // VehicleSeatIndex
        *data << uint32(transport.time);                                // MoveTime

        data->WriteBit(hasPrevTime);
        data->WriteBit(hasVehicleId);
        data->FlushBits();

        if (hasPrevTime)
            *data << uint32(transport.time2);                           // PrevMoveTime
        // hasVehicleId is always false in AC -> no VehicleRecID emitted
    }
}

void Object::BuildMovementUpdate(ByteBuffer* data, CreateObjectBits flags, Player* target) const
{
    std::vector<uint32> const* PauseTimes = nullptr;
    // AzerothCore GameObject has no GetPauseTimes(); transport-stop pause frames are unsupported.

    data->WriteBit(flags.NoBirthAnim);
    data->WriteBit(flags.EnablePortals);
    data->WriteBit(flags.PlayHoverAnim);
    data->WriteBit(flags.MovementUpdate);
    data->WriteBit(flags.MovementTransport);
    data->WriteBit(flags.Stationary);
    data->WriteBit(flags.CombatVictim);
    data->WriteBit(flags.ServerTime);
    data->WriteBit(flags.Vehicle);
    data->WriteBit(flags.AnimKit);
    data->WriteBit(flags.Rotation);
    data->WriteBit(flags.AreaTrigger);
    data->WriteBit(flags.GameObject);
    data->WriteBit(flags.SmoothPhasing);
    data->WriteBit(flags.ThisIsYou);
    data->WriteBit(flags.SceneObject);
    data->WriteBit(flags.ActivePlayer);
    data->WriteBit(flags.Conversation);
    data->FlushBits();

    if (flags.MovementUpdate)
    {
        Unit const* unit = ToUnit();
        bool HasFallDirection = unit->HasUnitMovementFlag(MOVEMENTFLAG_FALLING);
        bool HasFall = HasFallDirection || unit->m_movementInfo.fallTime != 0;
        bool HasSpline = unit->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_SPLINE_ENABLED);
        bool HasInertia = false;                                        // AC MovementInfo has no inertia
        bool HasAdvFlying = false;                                      // AC MovementInfo has no advFlying
        bool HasStandingOnGameObjectGUID = false;                       // AC MovementInfo has no standingOnGameObjectGUID

        *data << GetGUID();                                             // MoverGUID

        *data << uint32(unit->GetUnitMovementFlags());
        *data << uint32(unit->GetExtraUnitMovementFlags());
        *data << uint32(0);                                             // ExtraUnitMovementFlags2 (no AC data)

        *data << uint32(unit->m_movementInfo.time);                     // MoveTime
        *data << float(unit->GetPositionX());
        *data << float(unit->GetPositionY());
        *data << float(unit->GetPositionZ());
        *data << float(unit->GetOrientation());

        *data << float(unit->m_movementInfo.pitch);                     // Pitch
        *data << float(0.0f);                                           // StepUpStartElevation (no AC data)

        *data << uint32(0);                                             // RemoveForcesIDs.size()
        *data << uint32(0);                                             // MoveIndex

        //for (std::size_t i = 0; i < RemoveForcesIDs.size(); ++i)
        //    *data << ObjectGuid(RemoveForcesIDs);

        data->WriteBit(HasStandingOnGameObjectGUID);                    // HasStandingOnGameObjectGUID
        data->WriteBit(!unit->m_movementInfo.transport.guid.IsEmpty()); // HasTransport
        data->WriteBit(HasFall);                                        // HasFall
        data->WriteBit(HasSpline);                                      // HasSpline - marks that the unit uses spline movement
        data->WriteBit(false);                                          // HeightChangeFailed
        data->WriteBit(false);                                          // RemoteTimeValid
        data->WriteBit(HasInertia);                                     // HasInertia
        data->WriteBit(HasAdvFlying);                                   // HasAdvFlying

        if (!unit->m_movementInfo.transport.guid.IsEmpty())
            WriteCreateObjectMovementTransport(data, unit->m_movementInfo.transport);

        // HasStandingOnGameObjectGUID / HasInertia / HasAdvFlying are always false in AzerothCore (no such MovementInfo data).

        if (HasFall)
        {
            *data << uint32(unit->m_movementInfo.fallTime);             // Time
            *data << float(unit->m_movementInfo.jump.zspeed);           // JumpVelocity

            if (data->WriteBit(HasFallDirection))
            {
                *data << float(unit->m_movementInfo.jump.sinAngle);     // Direction
                *data << float(unit->m_movementInfo.jump.cosAngle);
                *data << float(unit->m_movementInfo.jump.xyspeed);      // Speed
            }
        }

        *data << float(unit->GetSpeed(MOVE_WALK));
        *data << float(unit->GetSpeed(MOVE_RUN));
        *data << float(unit->GetSpeed(MOVE_RUN_BACK));
        *data << float(unit->GetSpeed(MOVE_SWIM));
        *data << float(unit->GetSpeed(MOVE_SWIM_BACK));
        *data << float(unit->GetSpeed(MOVE_FLIGHT));
        *data << float(unit->GetSpeed(MOVE_FLIGHT_BACK));
        *data << float(unit->GetSpeed(MOVE_TURN_RATE));
        *data << float(unit->GetSpeed(MOVE_PITCH_RATE));

        *data << uint32(0);                                            // MovementForces count (AC has no MovementForces)
        *data << float(1.0f);                                          // MovementForcesModMagnitude

        *data << float(2.0f);                                           // advFlyingAirFriction
        *data << float(65.0f);                                          // advFlyingMaxVel
        *data << float(1.0f);                                           // advFlyingLiftCoefficient
        *data << float(3.0f);                                           // advFlyingDoubleJumpVelMod
        *data << float(10.0f);                                          // advFlyingGlideStartMinHeight
        *data << float(100.0f);                                         // advFlyingAddImpulseMaxSpeed
        *data << float(90.0f);                                          // advFlyingMinBankingRate
        *data << float(140.0f);                                         // advFlyingMaxBankingRate
        *data << float(180.0f);                                         // advFlyingMinPitchingRateDown
        *data << float(360.0f);                                         // advFlyingMaxPitchingRateDown
        *data << float(90.0f);                                          // advFlyingMinPitchingRateUp
        *data << float(270.0f);                                         // advFlyingMaxPitchingRateUp
        *data << float(30.0f);                                          // advFlyingMinTurnVelocityThreshold
        *data << float(80.0f);                                          // advFlyingMaxTurnVelocityThreshold
        *data << float(2.75f);                                          // advFlyingSurfaceFriction
        *data << float(7.0f);                                           // advFlyingOverMaxDeceleration
        *data << float(0.4f);                                           // advFlyingLaunchSpeedCoefficient

        data->WriteBit(HasSpline);
        data->FlushBits();

        if (HasSpline)
            Movement::PacketBuilder::WriteCreate(*unit->movespline, *data);
    }

    *data << uint32(PauseTimes ? PauseTimes->size() : 0);

    if (flags.Stationary)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        *data << float(self->GetStationaryX());
        *data << float(self->GetStationaryY());
        *data << float(self->GetStationaryZ());
        *data << float(self->GetStationaryO());
    }

    if (flags.CombatVictim)
        *data << ToUnit()->GetVictim()->GetGUID();                      // CombatVictim

    if (flags.ServerTime)
        *data << uint32(GameTime::GetGameTimeMS().count());

    if (flags.Vehicle)
    {
        Unit const* unit = ToUnit();
        *data << uint32(unit->GetVehicleKit()->GetVehicleInfo()->m_ID); // RecID
        *data << float(unit->GetOrientation());                         // InitialRawFacing
    }

    if (flags.AnimKit)
    {
        // AzerothCore has no AnimKit data on WorldObject; emit zeros to keep wire layout intact.
        *data << uint16(0);                                             // AiID
        *data << uint16(0);                                             // MovementID
        *data << uint16(0);                                             // MeleeID
    }

    if (flags.Rotation)
        *data << uint64(ToGameObject()->GetPackedWorldRotation());      // Rotation

    if (PauseTimes && !PauseTimes->empty())
        data->append(PauseTimes->data(), PauseTimes->size());

    if (flags.MovementTransport)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        WriteCreateObjectMovementTransport(data, self->m_movementInfo.transport);
    }

    // flags.AreaTrigger / flags.SmoothPhasing / flags.Conversation blocks removed:
    // AzerothCore has no AreaTrigger/SceneObject/Conversation entities and no SmoothPhasing,
    // so those CreateObjectBits are never set. Removing the bodies keeps the wire layout intact.

    if (flags.GameObject)
    {
        // AzerothCore GameObject has no WorldEffectID; emit defaults to preserve wire layout.
        *data << uint32(0);                                            // WorldEffectID

        data->WriteBit(false);                                        // bit8 (HasStateWorldEffectIDs)
        data->FlushBits();
    }

    if (flags.SceneObject)
    {
        data->WriteBit(false);                                          // HasLocalScriptData
        data->WriteBit(false);                                          // HasPetBattleFullUpdate
        data->FlushBits();
    }

    if (flags.ActivePlayer)
    {
        Player const* player = ToPlayer();

        bool HasSceneInstanceIDs = false;                               // AC has no SceneMgr
        bool HasRuneState = player->getClass() == CLASS_DEATH_KNIGHT;   // only Death Knights have rune state
        bool HasActionButtons = true;

        data->WriteBit(HasSceneInstanceIDs);
        data->WriteBit(HasRuneState);
        data->WriteBit(HasActionButtons);
        data->FlushBits();

        if (HasRuneState)
        {
            *data << uint8((1 << MAX_RUNES) - 1);
            *data << uint8(player->GetRunesState());
            *data << uint32(MAX_RUNES);
            for (uint8 i = 0; i < MAX_RUNES; ++i)
                *data << uint8((1.0f - float(player->GetRuneCooldown(i)) / float(RUNE_BASE_COOLDOWN)) * 255.0f);
        }
        if (HasActionButtons)
        {
            for (uint8 i = 0; i < MAX_ACTION_BUTTONS; ++i)
            {
                ActionButton const* button = const_cast<Player*>(player)->GetActionButton(i);
                if (button && button->uState != ACTIONBUTTON_DELETED)
                    *data << uint32(button->packedData);
                else
                    *data << uint32(0);
            }
        }
    }
}

UF::UpdateFieldFlag Object::GetUpdateFieldFlagsFor(Player const* /*target*/) const
{
    return UF::UpdateFieldFlag::None;
}

void Object::BuildValuesUpdateWithFlag(ByteBuffer* data, UF::UpdateFieldFlag /*flags*/, Player const* /*target*/) const
{
    std::size_t sizePos = data->wpos();
    *data << uint32(0);
    *data << uint32(0);

    data->put<uint32>(sizePos, data->wpos() - sizePos - 4);
}

void Object::AddToObjectUpdateIfNeeded()
{
    if (m_inWorld && !m_objectUpdated)
        m_objectUpdated = AddToObjectUpdate();
}

void Object::ClearUpdateMask(bool remove)
{
    m_values.ClearChangesMask(&Object::m_objectData);

    if (m_objectUpdated)
    {
        if (remove)
            RemoveFromObjectUpdate();
        m_objectUpdated = false;
    }
}

void Object::BuildFieldsUpdate(Player* player, UpdateDataMapType& data_map) const
{
    UpdateDataMapType::iterator iter = data_map.find(player);

    if (iter == data_map.end())
    {
        std::pair<UpdateDataMapType::iterator, bool> p = data_map.emplace(player, UpdateData(player->GetMapId()));
        ASSERT(p.second);
        iter = p.first;
    }

    BuildValuesUpdateBlockForPlayer(&iter->second, iter->first);
}

std::string Object::GetDebugInfo() const
{
    std::stringstream sstr;
    sstr << GetGUID().ToString() + " Entry " << GetEntry();
    return sstr.str();
}

UnitMoveType MovementInfo::GetSpeedType(uint32 moveFlags)
{
    if (moveFlags & MOVEMENTFLAG_FLYING)
    {
        if (moveFlags & MOVEMENTFLAG_BACKWARD)
            return MOVE_FLIGHT_BACK;

        return MOVE_FLIGHT;
    }
    else if (moveFlags & MOVEMENTFLAG_SWIMMING)
    {
        if (moveFlags & MOVEMENTFLAG_BACKWARD)
            return MOVE_SWIM_BACK;

        return MOVE_SWIM;
    }
    else if (moveFlags & MOVEMENTFLAG_WALKING)
        return MOVE_WALK;
    else if (moveFlags & MOVEMENTFLAG_BACKWARD)
        return MOVE_RUN_BACK;

    return MOVE_RUN;
}

void MovementInfo::OutDebug()
{
    LOG_INFO("movement", "MOVEMENT INFO");
    LOG_INFO("movement", "guid {}", guid.ToString());
    LOG_INFO("movement", "flags {}", flags);
    LOG_INFO("movement", "flags2 {}", flags2);
    LOG_INFO("movement", "time {} current time {}", flags2, uint64(::GameTime::GetGameTime().count()));
    LOG_INFO("movement", "position: `{}`", pos.ToString());

    if (flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        LOG_INFO("movement", "TRANSPORT:");
        LOG_INFO("movement", "guid: {}", transport.guid.ToString());
        LOG_INFO("movement", "position: `{}`", transport.pos.ToString());
        LOG_INFO("movement", "seat: {}", transport.seat);
        LOG_INFO("movement", "time: {}", transport.time);

        if (flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT)
        {
            LOG_INFO("movement", "time2: {}", transport.time2);
        }
    }

    if ((flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
        LOG_INFO("movement", "pitch: {}", pitch);

    LOG_INFO("movement", "fallTime: {}", fallTime);
    if (flags & MOVEMENTFLAG_FALLING)
        LOG_INFO("movement", "j_zspeed: {} j_sinAngle: {} j_cosAngle: {} j_xyspeed: {}", jump.zspeed, jump.sinAngle, jump.cosAngle, jump.xyspeed);

    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
        LOG_INFO("movement", "splineElevation: {}", splineElevation);
}

WorldObject::WorldObject() : WorldLocation(),
    LastUsedScriptID(0), m_name(""), m_isActive(false), _visibilityDistanceOverrideType(VisibilityDistanceType::Normal), m_zoneScript(nullptr),
    _zoneId(0), _areaId(0), _floorZ(INVALID_HEIGHT), _outdoors(false), _liquidData(), _updatePositionData(false), m_transport(nullptr),
    m_currMap(nullptr), _heartbeatTimer(HEARTBEAT_INTERVAL), m_InstanceId(0), m_phaseMask(PHASEMASK_NORMAL), m_useCombinedPhases(true),
    m_notifyflags(0), m_executed_notifies(0), _objectVisibilityContainer(this)
{
    m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE | GHOST_VISIBILITY_GHOST);
    m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);

    sScriptMgr->OnWorldObjectCreate(this);
}

void WorldObject::Update(uint32 diff)
{
    m_Events.Update(diff);

    _heartbeatTimer -= Milliseconds(diff);
    while (_heartbeatTimer <= 0ms)
    {
        _heartbeatTimer += HEARTBEAT_INTERVAL;
        Heartbeat();
    }

    sScriptMgr->OnWorldObjectUpdate(this, diff);
}

void WorldObject::setActive(bool on)
{
    if (m_isActive == on)
        return;

    if (IsPlayer())
        return;

    m_isActive = on;

    if (!on || !IsInWorld())
        return;

    Map* map = FindMap();
    if (!map)
        return;

    map->AddObjectToPendingUpdateList(this);
}

float WorldObject::GetVisibilityOverrideDistance() const
{
    ASSERT(_visibilityDistanceOverrideType < VisibilityDistanceType::Max);
    return VisibilityDistances[AsUnderlyingType(_visibilityDistanceOverrideType)];
}

void WorldObject::SetVisibilityDistanceOverride(VisibilityDistanceType type)
{
    ASSERT(type < VisibilityDistanceType::Max);

    if (type == GetVisibilityOverrideType())
        return;

    if (!IsCreature() && !IsGameObject() && !IsDynamicObject())
        return;

    // Important to remove from old visibility override containers first
    RemoveFromMapVisibilityOverrideContainers();

    // Always update _visibilityDistanceOverrideType, even when not in world
    _visibilityDistanceOverrideType = type;

    // Finally, add to new visibility override containers
    AddToMapVisibilityOverrideContainers();
}

void WorldObject::RemoveFromMapVisibilityOverrideContainers()
{
    if (!IsVisibilityOverridden())
        return;

    if (!IsInWorld())
        return;

    if (IsFarVisible())
        GetMap()->RemoveWorldObjectFromFarVisibleMap(this);
    else if (IsZoneWideVisible())
        GetMap()->RemoveWorldObjectFromZoneWideVisibleMap(_zoneId, this);
}

void WorldObject::AddToMapVisibilityOverrideContainers()
{
    if (!IsVisibilityOverridden())
        return;

    if (!IsInWorld())
        return;

    if (IsFarVisible())
        GetMap()->AddWorldObjectToFarVisibleMap(this);
    else if (IsZoneWideVisible())
        GetMap()->AddWorldObjectToZoneWideVisibleMap(_zoneId, this);
}

void WorldObject::CleanupsBeforeDelete(bool /*finalCleanup*/)
{
    if (IsInWorld())
        RemoveFromWorld();

    m_Events.KillAllEvents(false);                      // non-delatable (currently cast spells) will not deleted now but it will deleted at call in Map::RemoveAllObjectsInRemoveList
}

void WorldObject::SetPositionDataUpdate()
{
    _updatePositionData = true;

    // Calls immediately for charmed units
    if (IsCreature() && ToUnit()->IsCharmedOwnedByPlayerOrPlayer())
        UpdatePositionData();
}

void WorldObject::UpdatePositionData()
{
    _updatePositionData = false;

    PositionFullTerrainStatus data;
    GetMap()->GetFullTerrainStatusForPosition(GetPhaseMask(), GetPositionX(), GetPositionY(), GetPositionZ(), GetCollisionHeight(), data);
    ProcessPositionDataChanged(data);
}

void WorldObject::ProcessPositionDataChanged(PositionFullTerrainStatus const& data)
{
    uint32 const oldZoneId = _zoneId;

    _zoneId = _areaId = data.areaId;

    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(_areaId))
        if (area->ParentAreaID)
            _zoneId = area->ParentAreaID;

    _outdoors   = data.outdoors;
    _floorZ     = data.floorZ;
    _liquidData = data.liquidInfo;

    // Has zone ID changed?
    if (oldZoneId != _zoneId)
    {
        // If so, check if we are far visibility overridden object and refresh maps if needed.
        if (IsZoneWideVisible())
        {
            GetMap()->RemoveWorldObjectFromZoneWideVisibleMap(oldZoneId, this);
            GetMap()->AddWorldObjectToZoneWideVisibleMap(_zoneId, this);
        }
    }
}

void WorldObject::AddToWorld()
{
    Object::AddToWorld();
    GetMap()->GetZoneAndAreaId(GetPhaseMask(), _zoneId, _areaId, GetPositionX(), GetPositionY(), GetPositionZ());
    GetMap()->AddObjectToPendingUpdateList(this);

    if (IsZoneWideVisible())
        GetMap()->AddWorldObjectToZoneWideVisibleMap(_zoneId, this);
}

void WorldObject::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    RemoveFromMapVisibilityOverrideContainers();

    DestroyForVisiblePlayers();

    GetObjectVisibilityContainer().CleanVisibilityReferences();

    Object::RemoveFromWorld();
}

InstanceScript* WorldObject::GetInstanceScript() const
{
    Map* map = GetMap();
    return map->IsDungeon() ? map->ToInstanceMap()->GetInstanceScript() : nullptr;
}

float WorldObject::GetDistanceZ(WorldObject const* obj) const
{
    float dz = std::fabs(GetPositionZ() - obj->GetPositionZ());
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = dz - sizefactor;
    return (dist > 0 ? dist : 0);
}

bool WorldObject::_IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D, bool incOwnRadius, bool incTargetRadius) const
{
    float maxdist = dist2compare;
    if (incOwnRadius)
        maxdist += GetObjectSize();

    if (incTargetRadius)
        maxdist += obj->GetObjectSize();

    if (m_transport && obj->GetTransport() &&  obj->GetTransport()->GetGUID() == m_transport->GetGUID())
    {
        float dtx = m_movementInfo.transport.pos.m_positionX - obj->m_movementInfo.transport.pos.m_positionX;
        float dty = m_movementInfo.transport.pos.m_positionY - obj->m_movementInfo.transport.pos.m_positionY;
        float disttsq = dtx * dtx + dty * dty;
        if (is3D)
        {
            float dtz = m_movementInfo.transport.pos.m_positionZ - obj->m_movementInfo.transport.pos.m_positionZ;
            disttsq += dtz * dtz;
        }
        return disttsq < (maxdist * maxdist);
    }

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx * dx + dy * dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz * dz;
    }

    return distsq < maxdist * maxdist;
}

Position WorldObject::GetHitSpherePointFor(Position const& dest, Optional<float> collisionHeight, Optional<float> combatReach) const
{
    G3D::Vector3 vThis(GetPositionX(), GetPositionY(), GetPositionZ() + (collisionHeight ? *collisionHeight : GetCollisionHeight()));
    G3D::Vector3 vObj(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
    G3D::Vector3 contactPoint = vThis + (vObj - vThis).directionOrZero() * std::min(dest.GetExactDist(this), (combatReach ? *combatReach : GetCombatReach()));

    return Position(contactPoint.x, contactPoint.y, contactPoint.z, GetAngle(contactPoint.x, contactPoint.y));
}

float WorldObject::GetDistance(WorldObject const* obj) const
{
    float d = GetExactDist(obj) - GetObjectSize() - obj->GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

[[nodiscard]] float WorldObject::GetDistance(const Position& pos) const
{
    float d = GetExactDist(&pos) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

[[nodiscard]] float WorldObject::GetDistance(float x, float y, float z) const
{
    float d = GetExactDist(x, y, z) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(WorldObject const* obj) const
{
    float d = GetExactDist2d(obj) - GetObjectSize() - obj->GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

[[nodiscard]] float WorldObject::GetDistance2d(float x, float y) const
{
    float d = GetExactDist2d(x, y) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

bool WorldObject::IsSelfOrInSameMap(WorldObject const* obj) const
{
    if (this == obj)
    {
        return true;
    }

    return IsInMap(obj);
}

bool WorldObject::IsInMap(WorldObject const* obj) const
{
    if (obj)
    {
        return IsInWorld() && obj->IsInWorld() && (FindMap() == obj->FindMap());
    }

    return false;
}

[[nodiscard]] bool WorldObject::IsWithinDist3d(float x, float y, float z, float dist) const
{
    return IsInDist(x, y, z, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist3d(const Position* pos, float dist) const
{
    return IsInDist(pos, dist + GetObjectSize());
}

[[nodiscard]] bool WorldObject::IsWithinDist2d(float x, float y, float dist) const
{
    return IsInDist2d(x, y, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist2d(const Position* pos, float dist) const
{
    return IsInDist2d(pos, dist + GetObjectSize());
}

// Visibility always uses 2d checks, factors in self-object size already. Gameobjects will override this for custom calc
bool WorldObject::IsWithinSightRange(Position const& pos, float dist) const
{
    return IsInDist2d(&pos, dist + GetObjectSize());
}

// use only if you will sure about placing both object at same map
bool WorldObject::IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D, bool incOwnRadius, bool incTargetRadius) const
{
    return obj && _IsWithinDist(obj, dist2compare, is3D, incOwnRadius, incTargetRadius);
}

bool WorldObject::IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D, bool incOwnRadius, bool incTargetRadius) const
{
    return obj && IsInMap(obj) && InSamePhase(obj) && _IsWithinDist(obj, dist2compare, is3D, incOwnRadius, incTargetRadius);
}

bool WorldObject::IsWithinLOS(float ox, float oy, float oz, VMAP::ModelIgnoreFlags ignoreFlags, LineOfSightChecks checks) const
{
    if (IsInWorld())
    {
        oz += GetCollisionHeight();
        float x, y, z;
        if (IsPlayer())
        {
            GetPosition(x, y, z);
            z += GetCollisionHeight();
        }
        else
        {
            GetHitSpherePointFor({ ox, oy, oz }, x, y, z);
        }

        return GetMap()->isInLineOfSight(x, y, z, ox, oy, oz, GetPhaseMask(), checks, ignoreFlags);
    }
    return true;
}

bool WorldObject::IsWithinLOSInMap(WorldObject const* obj, VMAP::ModelIgnoreFlags ignoreFlags, LineOfSightChecks checks, Optional<float> collisionHeight /*= { }*/, Optional<float> combatReach /*= { }*/) const
{
   if (!IsInMap(obj))
        return false;

    float ox, oy, oz;
    if (obj->IsPlayer())
    {
        obj->GetPosition(ox, oy, oz);
        oz += obj->GetCollisionHeight();
    }
    else
        obj->GetHitSpherePointFor({ GetPositionX(), GetPositionY(), GetPositionZ() + (collisionHeight ? *collisionHeight : GetCollisionHeight()) }, ox, oy, oz);

    float x, y, z;
    if (IsPlayer())
    {
        GetPosition(x, y, z);
        z += GetCollisionHeight();
    }
    else
        GetHitSpherePointFor({ obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ() + obj->GetCollisionHeight() }, x, y, z, collisionHeight, combatReach);

    return GetMap()->isInLineOfSight(x, y, z, ox, oy, oz, GetPhaseMask(), checks, ignoreFlags);
}

void WorldObject::GetHitSpherePointFor(Position const& dest, float& x, float& y, float& z, Optional<float> collisionHeight, Optional<float> combatReach) const
{
    Position pos = GetHitSpherePointFor(dest, collisionHeight, combatReach);
    x = pos.GetPositionX();
    y = pos.GetPositionY();
    z = pos.GetPositionZ();
}

bool WorldObject::GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D /* = true */) const
{
    float dx1 = GetPositionX() - obj1->GetPositionX();
    float dy1 = GetPositionY() - obj1->GetPositionY();
    float distsq1 = dx1 * dx1 + dy1 * dy1;
    if (is3D)
    {
        float dz1 = GetPositionZ() - obj1->GetPositionZ();
        distsq1 += dz1 * dz1;
    }

    float dx2 = GetPositionX() - obj2->GetPositionX();
    float dy2 = GetPositionY() - obj2->GetPositionY();
    float distsq2 = dx2 * dx2 + dy2 * dy2;
    if (is3D)
    {
        float dz2 = GetPositionZ() - obj2->GetPositionZ();
        distsq2 += dz2 * dz2;
    }

    return distsq1 < distsq2;
}

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx * dx + dy * dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz * dz;
    }

    float sizefactor = GetObjectSize() + obj->GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange2d(float x, float y, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float distsq = dx * dx + dy * dy;

    float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange3d(float x, float y, float z, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float distsq = dx * dx + dy * dy + dz * dz;

    float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInBetween(WorldObject const* obj1, WorldObject const* obj2, float size) const
{
    if (!obj1 || !obj2)
        return false;

    if (!size)
        size = GetObjectSize() / 2;

    float pdist = obj1->GetExactDist2dSq(obj2) + size / 2.0f;
    if (GetExactDist2dSq(obj1) >= pdist || GetExactDist2dSq(obj2) >= pdist)
        return false;

    if (G3D::fuzzyEq(obj1->GetPositionX(), obj2->GetPositionX()))
        return GetPositionX() >= obj1->GetPositionX() - size && GetPositionX() <= obj1->GetPositionX() + size;

    float A = (obj2->GetPositionY() - obj1->GetPositionY()) / (obj2->GetPositionX() - obj1->GetPositionX());
    float B = -1;
    float C = obj1->GetPositionY() - A * obj1->GetPositionX();
    float dist = std::fabs(A * GetPositionX() + B * GetPositionY() + C) / std::sqrt(A * A + B * B);
    return dist <= size;
}

bool WorldObject::isInFront(WorldObject const* target,  float arc) const
{
    return HasInArc(arc, target);
}

bool WorldObject::isInBack(WorldObject const* target, float arc) const
{
    return !HasInArc(2 * M_PI - arc, target);
}

void WorldObject::GetRandomPoint(const Position& pos, float distance, float& rand_x, float& rand_y, float& rand_z) const
{
    if (!distance)
    {
        pos.GetPosition(rand_x, rand_y, rand_z);
        return;
    }

    // angle to face `obj` to `this`
    float angle = (float)rand_norm() * static_cast<float>(2 * M_PI);
    float new_dist = (float)rand_norm() * static_cast<float>(distance);

    rand_x = pos.m_positionX + new_dist * cos(angle);
    rand_y = pos.m_positionY + new_dist * std::sin(angle);
    rand_z = pos.m_positionZ;

    Acore::NormalizeMapCoord(rand_x);
    Acore::NormalizeMapCoord(rand_y);
    UpdateGroundPositionZ(rand_x, rand_y, rand_z);            // update to LOS height if available
}

Position WorldObject::GetRandomPoint(const Position& srcPos, float distance) const
{
    float x, y, z;
    GetRandomPoint(srcPos, distance, x, y, z);
    return Position(x, y, z, GetOrientation());
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = GetMapHeight(x, y, z);
    if (new_z > INVALID_HEIGHT)
        z = new_z + (IsUnit() ? static_cast<Unit const*>(this)->GetHoverHeight() : 0.0f);
}

/**
 * @brief Get the minimum height of a object that should be in water
 * to start floating/swim
 *
 * @return float
 */
float WorldObject::GetMinHeightInWater() const
{
    // have a fun with Archimedes' formula
    auto height = GetCollisionHeight();
    auto width = GetCollisionWidth();
    auto weight = getWeight(height, width, 1040); // avg human specific weight
    auto heightOutOfWater = getOutOfWater(width, weight, 10202) * 4.0f; // avg human density
    auto heightInWater = height - heightOutOfWater;
    return (height > heightInWater ? heightInWater : (height - (height / 3)));
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float& z, float* groundZ) const
{
    if (GetTransport())
    {
        if (groundZ)
            *groundZ = z;

        return;
    }

    if (Unit const* unit = ToUnit())
    {
        if (!unit->CanFly())
        {
            Creature const* c = unit->ToCreature();
            bool canSwim = c ? c->CanSwim() : true;
            float ground_z = z;
            float max_z;
            if (canSwim)
                max_z = GetMapWaterOrGroundLevel(x, y, z, &ground_z);
            else
                max_z = ground_z = GetMapHeight(x, y, z);

            if (max_z > INVALID_HEIGHT)
            {
                if (canSwim && unit->GetMap()->IsInWater(unit->GetPhaseMask(), x, y, max_z - Z_OFFSET_FIND_HEIGHT, unit->GetCollisionHeight()))
                {
                    // do not allow creatures to walk on
                    // water level while swimming
                    max_z = std::max(max_z - GetMinHeightInWater(), ground_z);
                }
                else
                {
                    // hovering units cannot go below their hover height
                    float hoverOffset = unit->GetHoverHeight();
                    max_z += hoverOffset;
                    ground_z += hoverOffset;
                }

                if (z > max_z)
                    z = max_z;
                else if (z < ground_z)
                    z = ground_z;
            }

            if (groundZ)
                *groundZ = ground_z;
        }
        else
        {
            float ground_z = GetMapHeight(x, y, z) + unit->GetHoverHeight();
            if (z < ground_z)
                z = ground_z;

            if (groundZ)
               *groundZ = ground_z;
        }
    }
    else
    {
        float ground_z = GetMapHeight(x, y, z);
        if (ground_z > INVALID_HEIGHT)
            z = ground_z;

        if (groundZ)
            *groundZ = ground_z;
    }
}

float WorldObject::GetGridActivationRange() const
{
    if (ToPlayer())
    {
        if (ToPlayer()->GetCinematicMgr().IsOnCinematic())
        {
            return DEFAULT_VISIBILITY_INSTANCE;
        }
        return IsInWintergrasp() ? VISIBILITY_DIST_WINTERGRASP : GetMap()->GetVisibilityRange();
    }
    else if (ToCreature())
    {
        return ToCreature()->m_SightDistance;
    }
    else if (((IsGameObject() && ToGameObject()->IsTransport()) || IsDynamicObject()) && isActiveObject())
    {
        return GetMap()->GetVisibilityRange();
    }

    return 0.0f;
}

float WorldObject::GetVisibilityRange() const
{
    if (IsCreature() && IsVisibilityOverridden())
        return GetVisibilityOverrideDistance();
    else if (IsGameObject())
    {
        if (IsInWintergrasp())
            return VISIBILITY_DIST_WINTERGRASP;
        else if (IsVisibilityOverridden())
            return GetVisibilityOverrideDistance();
        else
            return GetMap()->GetVisibilityRange();
    }
    else
        return IsInWintergrasp() ? VISIBILITY_DIST_WINTERGRASP : GetMap()->GetVisibilityRange();
}

float WorldObject::GetSightRange(WorldObject const* target) const
{
    if (ToUnit())
    {
        if (ToPlayer())
        {
            if (target)
            {
                if (target->IsCreature() && target->IsVisibilityOverridden())
                    return target->GetVisibilityOverrideDistance();
                else if (target->IsGameObject())
                {
                    if (IsInWintergrasp() && target->IsInWintergrasp())
                        return VISIBILITY_DIST_WINTERGRASP;
                    else if (target->IsVisibilityOverridden())
                        return target->GetVisibilityOverrideDistance();
                    else if (ToPlayer()->GetCinematicMgr().IsOnCinematic())
                        return DEFAULT_VISIBILITY_INSTANCE;
                    else
                        return GetMap()->GetVisibilityRange();
                }

                return IsInWintergrasp() && target->IsInWintergrasp() ? VISIBILITY_DIST_WINTERGRASP : GetMap()->GetVisibilityRange();
            }
            return IsInWintergrasp() ? VISIBILITY_DIST_WINTERGRASP : GetMap()->GetVisibilityRange();
        }
        else if (ToCreature())
            return ToCreature()->m_SightDistance;
        else
            return SIGHT_RANGE_UNIT;
    }

    if (ToDynObject() && isActiveObject())
        return GetMap()->GetVisibilityRange();

    return 0.0f;
}

bool WorldObject::CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth, bool distanceCheck, bool checkAlert) const
{
    if (this == obj)
        return true;

    if (CanNeverSee(obj))
        return false;

    if (obj->IsAlwaysVisibleFor(this) || CanAlwaysSee(obj))
        return true;

    // Creature scripts
    if (Creature const* cObj = obj->ToCreature())
    {
        if (Player const* player = ToPlayer())
        {
            if (cObj->IsAIEnabled && !cObj->AI()->CanBeSeen(player))
                return false;

            if (!player->CanSeeObjectByVisibilityConditions(obj))
                return false;
        }
    }

    // Gameobject scripts
    if (GameObject const* goObj = obj->ToGameObject())
    {
        if (Player const* player = ToPlayer())
        {
            if (!goObj->AI()->CanBeSeen(player))
                return false;

            if (!player->CanSeeObjectByVisibilityConditions(obj))
                return false;
        }
    }

    // pussywizard: arena spectator
    if (obj->IsPlayer())
        if (((Player const*)obj)->IsSpectator() && ((Player const*)obj)->FindMap() && ((Player const*)obj)->FindMap()->IsBattleArena())
            return false;

    bool corpseVisibility = false;
    if (distanceCheck)
    {
        bool corpseCheck = false;
        Position const* sightPosition = this;
        if (Player const* thisPlayer = ToPlayer())
        {
            sightPosition = &thisPlayer->GetSightPosition();

            if (Creature const* creature = obj->ToCreature())
            {
                if (TempSummon const* tempSummon = creature->ToTempSummon())
                {
                    if (tempSummon->IsVisibleBySummonerOnly() && GetGUID() != tempSummon->GetSummonerGUID())
                    {
                        return false;
                    }
                }
            }

            if (thisPlayer->isDead() && thisPlayer->GetHealth() > 0 && // Cheap way to check for ghost state
                    !(obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & GHOST_VISIBILITY_GHOST))
            {
                if (Corpse* corpse = thisPlayer->GetCorpse())
                {
                    corpseCheck = true;
                    if (corpse->IsWithinDist(thisPlayer, GetSightRange(obj), false))
                        if (corpse->IsWithinDist(obj, GetSightRange(obj), false))
                            corpseVisibility = true;
                }
            }

            // our additional checks
            if (Unit const* target = obj->ToUnit())
            {
                // xinef: don't allow to detect vehicle accessory if you can't see vehicle base!
                if (Unit const* vehicle = target->GetVehicleBase())
                    if (!thisPlayer->HaveAtClient(vehicle))
                        return false;

                // pussywizard: during arena preparation, don't allow to detect pets if can't see its owner (spoils enemy arena frames)
                if (Map* targetMap = target->FindMap())
                    if (target->IsPet() && target->GetOwnerGUID() && targetMap->IsBattleArena() && GetGUID() != target->GetOwnerGUID())
                        if (BattlegroundMap* bgmap = targetMap->ToBattlegroundMap())
                            if (Battleground* bg = bgmap->GetBG())
                                if (bg->GetStatus() < STATUS_IN_PROGRESS && !thisPlayer->HaveAtClient(target->GetOwnerGUID()))
                                    return false;
            }

            if (thisPlayer->GetFarSightDistance() && !thisPlayer->isInFront(obj))
                return false;
        }

        if (!corpseCheck && !obj->IsWithinSightRange(*sightPosition, GetSightRange(obj)))
            return false;
    }

    // GM visibility off or hidden NPC
    if (!obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM))
    {
        // Stop checking other things for GMs
        if (m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GM))
            return true;
    }
    else
        return m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GM) >= obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM);

    // Ghost players, Spirit Healers, and some other NPCs
    if (!corpseVisibility && !(obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GHOST)))
    {
        // Alive players can see dead players in some cases, but other objects can't do that
        if (Player const* thisPlayer = ToPlayer())
        {
            if (Player const* objPlayer = obj->ToPlayer())
            {
                if (thisPlayer->GetTeamId() != objPlayer->GetTeamId() || !thisPlayer->IsGroupVisibleFor(objPlayer))
                    return false;
            }
            else
                return false;
        }
        else
            return false;
    }

    if (obj->IsInvisibleDueToDespawn())
        return false;

    // pussywizard: arena spectator
    if (this->IsPlayer())
        if (((Player const*)this)->IsSpectator() && ((Player const*)this)->FindMap() && ((Player const*)this)->FindMap()->IsBattleArena() && (obj->m_invisibility.GetFlags() || obj->m_stealth.GetFlags()))
            return false;

    if (!CanDetect(obj, ignoreStealth, !distanceCheck, checkAlert))
    {
        return false;
    }

    return true;
}

bool WorldObject::CanNeverSee(WorldObject const* obj) const
{
    if (!IsInWorld())
        return true;

    if (obj->IsNeverVisible())
        return true;

    if (IsCreature() && obj->IsCreature())
        return GetMap() != obj->GetMap() || (!InSamePhase(obj) && ToUnit()->GetVehicleBase() != obj && this != obj->ToUnit()->GetVehicleBase());
    return GetMap() != obj->GetMap() || !InSamePhase(obj);
}

bool WorldObject::CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkClient, bool checkAlert) const
{
    WorldObject const* seer = this;

    // Pets don't have detection, they use the detection of their masters
    if (Unit const* thisUnit = ToUnit())
        if (Unit* controller = thisUnit->GetCharmerOrOwner())
            seer = controller;

    if (obj->IsAlwaysDetectableFor(seer) || GetEntry() == WORLD_TRIGGER) // xinef: World Trigger can detect all objects, used for wild gameobjects without owner!
        return true;

    if (!ignoreStealth)
    {
        if (!seer->CanDetectInvisibilityOf(obj)) // xinef: added ignoreStealth, allow AoE spells to hit invisible targets!
        {
            return false;
        }

        if (!seer->CanDetectStealthOf(obj, checkAlert))
        {
            // xinef: ignore units players have at client, this cant be cheated!
            if (checkClient)
            {
                if (!IsPlayer() || !ToPlayer()->HaveAtClient(obj))
                    return false;
            }
            else
                return false;
        }
    }

    return true;
}

bool WorldObject::CanDetectInvisibilityOf(WorldObject const* obj) const
{
    uint32 mask = obj->m_invisibility.GetFlags() & m_invisibilityDetect.GetFlags();
    // xinef: include invisible flags of caster in the mask, 2 invisible objects should be able to detect eachother
    mask |= obj->m_invisibility.GetFlags() & m_invisibility.GetFlags();

    // Check for not detected types
    if (mask != obj->m_invisibility.GetFlags())
        return false;

    // It isn't possible in invisibility to detect something that can't detect the invisible object
    // (it's at least true for spell: 66)
    // It seems like that only Units are affected by this check (couldn't see arena doors with preparation invisibility)
    if (obj->ToUnit())
    {
        // Permanently invisible creatures should be able to engage non-invisible targets.
        // ex. Skulking Witch (20882) / Greater Invisibility (16380)
        bool isPermInvisibleCreature = false;
        if (Creature const* baseObj = ToCreature())
        {
            Unit::AuraEffectList const& auraEffects = baseObj->GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
            for (AuraEffect* const effect : auraEffects)
            {
                if (SpellInfo const* spell = effect->GetSpellInfo())
                {
                    if (spell->GetMaxDuration() == -1)
                    {
                        isPermInvisibleCreature = true;
                    }
                }
            }
        }

        if (!isPermInvisibleCreature)
        {
            uint32 objMask = m_invisibility.GetFlags() & obj->m_invisibilityDetect.GetFlags();
            // xinef: include invisible flags of caster in the mask, 2 invisible objects should be able to detect eachother
            objMask |= m_invisibility.GetFlags() & obj->m_invisibility.GetFlags();
            if (objMask != m_invisibility.GetFlags())
                return false;
        }
    }

    for (uint32 i = 0; i < TOTAL_INVISIBILITY_TYPES; ++i)
    {
        if (!(mask & (1 << i)))
            continue;

        // xinef: visible for the same invisibility type:
        if (m_invisibility.GetValue(InvisibilityType(i)) && obj->m_invisibility.GetValue(InvisibilityType(i)))
            continue;

        int32 objInvisibilityValue = obj->m_invisibility.GetValue(InvisibilityType(i));
        int32 ownInvisibilityDetectValue = m_invisibilityDetect.GetValue(InvisibilityType(i));

        // Too low value to detect
        if (ownInvisibilityDetectValue < objInvisibilityValue)
            return false;
    }

    return true;
}

bool WorldObject::CanDetectStealthOf(WorldObject const* obj, bool checkAlert) const
{
    // Combat reach is the minimal distance (both in front and behind),
    //   and it is also used in the range calculation.
    // One stealth point increases the visibility range by 0.3 yard.

    if (!obj->m_stealth.GetFlags())
        return true;

    // dead players shouldnt be able to detect stealth on arenas
    if (isType(TYPEMASK_PLAYER))
        if (!ToPlayer()->IsAlive())
            return false;

    float distance = GetExactDist(obj);
    float combatReach = 0.0f;

    if (IsUnit())
        combatReach = ((Unit*)this)->GetCombatReach();

    if (distance < combatReach)
        return true;

    if (!HasInArc(M_PI, obj))
        return false;

    for (uint32 i = 0; i < TOTAL_STEALTH_TYPES; ++i)
    {
        if (!(obj->m_stealth.GetFlags() & (1 << i)))
            continue;

        if (IsUnit())
            if (((Unit*)this)->HasAuraTypeWithMiscvalue(SPELL_AURA_DETECT_STEALTH, i))
                return true;

        // Starting points
        int32 detectionValue = 30;

        // Level difference: 5 point / level, starting from level 1.
        // There may be spells for this and the starting points too, but
        // not in the DBCs of the client.
        detectionValue += int32(getLevelForTarget(obj) - 1) * 5;

        // Apply modifiers
        detectionValue += m_stealthDetect.GetValue(StealthType(i));
        if (obj->isType(TYPEMASK_GAMEOBJECT))
        {
            detectionValue += 30; // pussywizard: increase detection range for gameobjects (ie. traps)
            if (Unit* owner = ((GameObject*)obj)->GetOwner())
                detectionValue -= int32(owner->getLevelForTarget(this) - 1) * 5;
        }

        detectionValue -= obj->m_stealth.GetValue(StealthType(i));

        // Calculate max distance
        float visibilityRange = float(detectionValue) * 0.3f + combatReach;

        Unit const* unit = ToUnit();

        // If this unit is an NPC then player detect range doesn't apply
        if (unit && unit->IsPlayer() && visibilityRange > MAX_PLAYER_STEALTH_DETECT_RANGE)
            visibilityRange = MAX_PLAYER_STEALTH_DETECT_RANGE;

        if (checkAlert)
            visibilityRange += (visibilityRange * 0.08f) + 1.5f;

        Unit const* targetUnit = obj->ToUnit();

        // If checking for alert, and creature's visibility range is greater than aggro distance, No alert
        if (checkAlert && unit && unit->ToCreature() && visibilityRange >= unit->ToCreature()->GetAttackDistance(targetUnit) + unit->ToCreature()->m_CombatDistance)
            return false;

        if (distance > visibilityRange)
            return false;
    }

    return true;
}

void WorldObject::SendPlayMusic(uint32 Music, bool OnlySelf)
{
    WorldPacket data(SMSG_PLAY_MUSIC, 4);
    data << Music;
    if (OnlySelf && IsPlayer())
        ToPlayer()->SendDirectMessage(&data);
    else
        SendMessageToSet(&data, true); // ToSelf ignored in this case
}

void Unit::BuildHeartBeatMsg(WorldPacket* data) const
{
    // TODO(3.4.3 brick-B): heartbeat broadcast redesigned in 3.4.3 — observers receive SMSG_MOVE_UPDATE
    // (MoveUpdate packet body differs from legacy). Verify in Phase-1d.
    data->Initialize(SMSG_MOVE_UPDATE, 32);
    *data << GetPackGUID();
    BuildMovementPacket(data);
}

void WorldObject::SendMessageToSet(WorldPacket const* data, bool self) const
{
    if (IsInWorld())
        SendMessageToSetInRange(data, 0.0f, self);
}

void WorldObject::SendMessageToSetInRange(WorldPacket const* data, float dist, bool /*self*/) const
{
    Acore::MessageDistDeliverer notifier(this, data, dist);
    notifier.Visit(GetObjectVisibilityContainer().GetVisiblePlayersMap());
}

void WorldObject::SendMessageToSet(WorldPacket const* data, Player const* skipped_rcvr) const
{
    Acore::MessageDistDeliverer notifier(this, data, 0.0f, false, skipped_rcvr);
    notifier.Visit(GetObjectVisibilityContainer().GetVisiblePlayersMap());
}

void WorldObject::SendObjectDeSpawnAnim(ObjectGuid guid)
{
    WorldPacket data(SMSG_GAME_OBJECT_DESPAWN, 8);
    data << guid;
    SendMessageToSet(&data, true);
}

void WorldObject::SetMap(Map* map)
{
    ASSERT(map);
    ASSERT(!IsInWorld());

    if (m_currMap == map) // command add npc: first create, than loadfromdb
    {
        return;
    }

    if (m_currMap)
    {
        LOG_FATAL("entities.object", "WorldObject::SetMap: obj {} new map {} {}, old map {} {}", (uint32)GetTypeId(), map->GetId(), map->GetInstanceId(), m_currMap->GetId(), m_currMap->GetInstanceId());
        ABORT();
    }

    m_currMap = map;
    m_mapId = map->GetId();
    m_InstanceId = map->GetInstanceId();

    sScriptMgr->OnWorldObjectSetMap(this, map);
}

void WorldObject::ResetMap()
{
    ASSERT(m_currMap);
    ASSERT(!IsInWorld());

    sScriptMgr->OnWorldObjectResetMap(this);

    m_currMap = nullptr;
    //maybe not for corpse
    //m_mapId = 0;
    //m_InstanceId = 0;
}

void WorldObject::AddObjectToRemoveList()
{
    Map* map = FindMap();
    if (!map)
    {
        LOG_ERROR("entities.object", "Object {} at attempt add to move list not have valid map (Id: {}).", GetGUID().ToString(), GetMapId());
        return;
    }

    map->AddObjectToRemoveList(this);
}

TempSummon* Map::SummonCreature(uint32 entry, Position const& pos, SummonPropertiesEntry const* properties /*= nullptr*/, uint32 duration /*= 0*/, WorldObject* summoner /*= nullptr*/, uint32 spellId /*= 0*/, uint32 vehId /*= 0*/, bool visibleBySummonerOnly /*= false*/)
{
    uint32 mask = UNIT_MASK_SUMMON;
    if (properties)
    {
        switch (properties->Category)
        {
            case SUMMON_CATEGORY_PET:
                mask = UNIT_MASK_GUARDIAN;
                break;
            case SUMMON_CATEGORY_PUPPET:
                mask = UNIT_MASK_PUPPET;
                break;
            case SUMMON_CATEGORY_VEHICLE:
                mask = UNIT_MASK_MINION;
                break;
            case SUMMON_CATEGORY_WILD:
            case SUMMON_CATEGORY_ALLY:
            case SUMMON_CATEGORY_UNK:
                {
                    switch (properties->Type)
                    {
                        case SUMMON_TYPE_MINION:
                        case SUMMON_TYPE_GUARDIAN:
                        case SUMMON_TYPE_GUARDIAN2:
                            mask = UNIT_MASK_GUARDIAN;
                            break;
                        case SUMMON_TYPE_TOTEM:
                        case SUMMON_TYPE_LIGHTWELL:
                            mask = UNIT_MASK_TOTEM;
                            break;
                        case SUMMON_TYPE_VEHICLE:
                        case SUMMON_TYPE_VEHICLE2:
                            mask = UNIT_MASK_SUMMON;
                            break;
                        case SUMMON_TYPE_MINIPET:
                        case SUMMON_TYPE_JEEVES:
                            mask = UNIT_MASK_MINION;
                            break;
                        default:
                            if (properties->Flags & 512) // Mirror Image, Summon Gargoyle
                                mask = UNIT_MASK_GUARDIAN;
                            break;
                    }
                    break;
                }
            default:
                return nullptr;
        }
    }

    uint32 phase = PHASEMASK_NORMAL;
    if (summoner)
        phase = summoner->GetPhaseMask();

    TempSummon* summon = nullptr;
    switch (mask)
    {
        case UNIT_MASK_SUMMON:
            summon = new TempSummon(properties, summoner ? summoner->GetGUID() : ObjectGuid::Empty);
            break;
        case UNIT_MASK_GUARDIAN:
            summon = new Guardian(properties, summoner ? summoner->GetGUID() : ObjectGuid::Empty);
            break;
        case UNIT_MASK_PUPPET:
            summon = new Puppet(properties, summoner ? summoner->GetGUID() : ObjectGuid::Empty);
            break;
        case UNIT_MASK_TOTEM:
            summon = new Totem(properties, summoner ? summoner->GetGUID() : ObjectGuid::Empty);
            break;
        case UNIT_MASK_MINION:
            summon = new Minion(properties, summoner ? summoner->GetGUID() : ObjectGuid::Empty);
            break;
        default:
            return nullptr;
    }

    EnsureGridLoaded(Cell(pos.GetPositionX(), pos.GetPositionY()));
    if (!summon->Create(GenerateLowGuid<HighGuid::Unit>(), this, phase, entry, vehId, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()))
    {
        delete summon;
        return nullptr;
    }

    summon->SetCreatedBySpell(spellId);

    summon->SetHomePosition(pos);

    summon->InitStats(duration);

    summon->SetVisibleBySummonerOnly(visibleBySummonerOnly);

    bool summonerHasTransport = summoner && summoner->GetTransport();
    bool summonerIsVehicle = summoner && summoner->IsUnit() && summoner->ToUnit()->IsVehicle();
    bool checkTransport = summon->GetOwnerGUID().IsPlayer() || (summonerHasTransport && !summonerIsVehicle);
    if (!AddToMap(summon->ToCreature(), checkTransport))
    {
        delete summon;
        return nullptr;
    }

    summon->InitSummon();

    // call MoveInLineOfSight for nearby creatures
    Acore::AIRelocationNotifier notifier(*summon);
    Cell::VisitObjects(summon, notifier, GetVisibilityRange());

    return summon;
}

/**
* Summons group of creatures.
*
* @param group Id of group to summon.
* @param list  List to store pointers to summoned creatures.
*/

void Map::SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list /*= nullptr*/)
{
    std::vector<TempSummonData> const* data = sObjectMgr->GetSummonGroup(GetId(), SUMMONER_TYPE_MAP, group);
    if (!data)
        return;

    for (std::vector<TempSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (TempSummon* summon = SummonCreature(itr->entry, itr->pos, nullptr, itr->time))
            if (list)
                list->push_back(summon);
}

void Map::SummonGameObjectGroup(uint8 group, std::list<GameObject*>* list /*= nullptr*/)
{
    std::vector<GameObjectSummonData> const* data = sObjectMgr->GetGameObjectSummonGroup(GetId(), SUMMONER_TYPE_MAP, group);
    if (!data)
        return;

    for (std::vector<GameObjectSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (GameObject* go = SummonGameObject(itr->entry, itr->pos.GetPositionX(), itr->pos.GetPositionY(), itr->pos.GetPositionZ(), itr->pos.GetOrientation(), itr->rot.x, itr->rot.y, itr->rot.z, itr->rot.w, itr->respawnTime))
            if (list)
                list->push_back(go);
}

TempSummon* WorldObject::SummonCreature(uint32 id, float x, float y, float z, float ang, TempSummonType spwtype, uint32 despwtime, SummonPropertiesEntry const* properties, bool visibleBySummonerOnly)
{
    if (!x && !y && !z)
    {
        GetClosePoint(x, y, z, GetObjectSize());
        ang = GetOrientation();
    }
    Position pos;
    pos.Relocate(x, y, z, ang);
    return SummonCreature(id, pos, spwtype, despwtime, 0, properties, visibleBySummonerOnly);
}

GameObject* Map::SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime, bool checkTransport)
{
    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);
    if (!goinfo)
    {
        LOG_ERROR("sql.sql", "Gameobject template {} not found in database!", entry);
        return nullptr;
    }

    GameObject* go = sObjectMgr->IsGameObjectStaticTransport(entry) ? new StaticTransport() : new GameObject();
    if (!go->Create(GenerateLowGuid<HighGuid::GameObject>(), entry, this, PHASEMASK_NORMAL, x, y, z, ang, G3D::Quat(rotation0, rotation1, rotation2, rotation3), 100, GO_STATE_READY))
    {
        delete go;
        return nullptr;
    }

    // Xinef: if gameobject is temporary, set custom spellid
    if (respawnTime)
        go->SetSpellId(1);

    go->SetRespawnTime(respawnTime);
    go->SetSpawnedByDefault(false);
    AddToMap(go, checkTransport);
    return go;
}

GameObject* Map::SummonGameObject(uint32 entry, Position const& pos, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime, bool checkTransport)
{
    return SummonGameObject(entry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), rotation0, rotation1, rotation2, rotation3, respawnTime, checkTransport);
}

void WorldObject::SetZoneScript()
{
    if (Map* map = FindMap())
    {
        if (InstanceMap* instanceMap = map->ToInstanceMap())
            m_zoneScript = reinterpret_cast<ZoneScript*>(instanceMap->GetInstanceScript());
        else if (!map->IsBattlegroundOrArena())
        {
            uint32 zoneId = GetZoneId();
            if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(zoneId))
                m_zoneScript = bf;
            else
                m_zoneScript = sOutdoorPvPMgr->GetZoneScript(zoneId);
        }
    }
}

void WorldObject::ClearZoneScript()
{
    m_zoneScript = nullptr;
}

TempSummon* WorldObject::SummonCreature(uint32 entry, const Position& pos, TempSummonType spwtype, uint32 duration, uint32  /*vehId*/, SummonPropertiesEntry const* properties, bool visibleBySummonerOnly /*= false*/) const
{
    if (Map* map = FindMap())
    {
        if (TempSummon* summon = map->SummonCreature(entry, pos, properties, duration, (WorldObject*)this, 0, 0, visibleBySummonerOnly))
        {
            summon->SetTempSummonType(spwtype);
            return summon;
        }
    }

    return nullptr;
}

GameObject* WorldObject::SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime, bool checkTransport, GOSummonType summonType)
{
    if (!IsInWorld())
        return nullptr;

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);
    if (!goinfo)
    {
        LOG_ERROR("sql.sql", "Gameobject template {} not found in database!", entry);
        return nullptr;
    }

    Map* map = GetMap();
    GameObject* go = sObjectMgr->IsGameObjectStaticTransport(entry) ? new StaticTransport() : new GameObject();
    if (!go->Create(map->GenerateLowGuid<HighGuid::GameObject>(), entry, map, GetPhaseMask(), x, y, z, ang, G3D::Quat(rotation0, rotation1, rotation2, rotation3), 100, GO_STATE_READY))
    {
        delete go;
        return nullptr;
    }

    go->SetRespawnTime(respawnTime);

    // Xinef: if gameobject is temporary, set custom spellid
    if (respawnTime)
        go->SetSpellId(1);

    if (IsPlayer() || (IsCreature() && summonType == GO_SUMMON_TIMED_OR_CORPSE_DESPAWN)) //not sure how to handle this
        ToUnit()->AddGameObject(go);
    else
        go->SetSpawnedByDefault(false);

    map->AddToMap(go, checkTransport);
    return go;
}

Creature* WorldObject::SummonTrigger(float x, float y, float z, float ang, uint32 duration, bool setLevel, CreatureAI * (*GetAI)(Creature*))
{
    TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Creature* summon = SummonCreature(WORLD_TRIGGER, x, y, z, ang, summonType, duration);
    if (!summon)
        return nullptr;

    //summon->SetName(GetName());
    if (setLevel && (IsPlayer() || IsCreature()))
    {
        summon->SetFaction(((Unit*)this)->GetFaction());
        summon->SetLevel(((Unit*)this)->GetLevel());
    }

    // Xinef: correctly set phase mask in case of gameobjects
    summon->SetPhaseMask(GetPhaseMask(), false);

    if (GetAI)
        summon->AIM_Initialize(GetAI(summon));
    return summon;
}

/**
* Summons group of creatures. Should be called only by instances of Creature and GameObject classes.
*
* @param group Id of group to summon.
* @param list  List to store pointers to summoned creatures.
*/
void WorldObject::SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list /*= nullptr*/)
{
    ASSERT((IsGameObject() || IsCreature()) && "Only GOs and creatures can summon npc groups!");

    std::vector<TempSummonData> const* data = sObjectMgr->GetSummonGroup(GetEntry(), IsGameObject() ? SUMMONER_TYPE_GAMEOBJECT : SUMMONER_TYPE_CREATURE, group);
    if (!data)
        return;

    for (std::vector<TempSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (TempSummon* summon = SummonCreature(itr->entry, itr->pos, itr->type, itr->time))
            if (list)
                list->push_back(summon);
}

void WorldObject::SummonGameObjectGroup(uint8 group, std::list<GameObject*>* list /*= nullptr*/)
{
    ASSERT((IsGameObject() || IsCreature()) && "Only GOs and creatures can summon gameobject groups!");

    std::vector<GameObjectSummonData> const* data = sObjectMgr->GetGameObjectSummonGroup(GetEntry(), IsGameObject() ? SUMMONER_TYPE_GAMEOBJECT : SUMMONER_TYPE_CREATURE, group);
    if (!data)
        return;

    for (std::vector<GameObjectSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (GameObject* go = SummonGameObject(itr->entry, itr->pos.GetPositionX(), itr->pos.GetPositionY(), itr->pos.GetPositionZ(), itr->pos.GetOrientation(), itr->rot.x, itr->rot.y, itr->rot.z, itr->rot.w, itr->respawnTime))
            if (list)
                list->push_back(go);
}

Creature* WorldObject::FindNearestCreature(uint32 entry, float range, bool alive) const
{
    Creature* creature = nullptr;
    Acore::NearestCreatureEntryWithLiveStateInObjectRangeCheck checker(*this, entry, alive, range);
    Acore::CreatureLastSearcher<Acore::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(this, creature, checker);
    Cell::VisitObjects(this, searcher, range);
    return creature;
}

GameObject* WorldObject::FindNearestGameObject(uint32 entry, float range, bool onlySpawned /*= false*/) const
{
    GameObject* go = nullptr;
    Acore::NearestGameObjectEntryInObjectRangeCheck checker(*this, entry, range, onlySpawned);
    Acore::GameObjectLastSearcher<Acore::NearestGameObjectEntryInObjectRangeCheck> searcher(this, go, checker);
    Cell::VisitObjects(this, searcher, range);
    return go;
}

GameObject* WorldObject::FindNearestGameObjectOfType(GameobjectTypes type, float range) const
{
    GameObject* go = nullptr;
    Acore::NearestGameObjectTypeInObjectRangeCheck checker(*this, type, range);
    Acore::GameObjectLastSearcher<Acore::NearestGameObjectTypeInObjectRangeCheck> searcher(this, go, checker);
    Cell::VisitObjects(this, searcher, range);
    return go;
}

Player* WorldObject::SelectNearestPlayer(float distance) const
{
    Player* target = nullptr;

    Acore::NearestPlayerInObjectRangeCheck checker(this, distance);
    Acore::PlayerLastSearcher<Acore::NearestPlayerInObjectRangeCheck> searcher(this, target, checker);
    Cell::VisitObjects(this, searcher, distance);

    return target;
}

std::string WorldObject::GetDebugInfo() const
{
    std::stringstream sstr;
    sstr << WorldLocation::GetDebugInfo() << "\n"
        << Object::GetDebugInfo() << "\n"
        << "Name: " << GetName();
    return sstr.str();
}

void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>& gameobjectList, uint32 entry, float maxSearchRange) const
{
    Acore::AllGameObjectsWithEntryInRange check(this, entry, maxSearchRange);
    Acore::GameObjectListSearcher<Acore::AllGameObjectsWithEntryInRange> searcher(this, gameobjectList, check);
    Cell::VisitObjects(this, searcher, maxSearchRange);
}

void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>& gameobjectList, std::vector<uint32> const& entries, float maxSearchRange) const
{
    Acore::AllGameObjectsMatchingOneEntryInRange check(this, entries, maxSearchRange);
    Acore::GameObjectListSearcher searcher(this, gameobjectList, check);
    Cell::VisitObjects(this, searcher, maxSearchRange);
}

void WorldObject::GetCreatureListWithEntryInGrid(std::list<Creature*>& creatureList, uint32 entry, float maxSearchRange) const
{
    Acore::AllCreaturesOfEntryInRange check(this, entry, maxSearchRange);
    Acore::CreatureListSearcher<Acore::AllCreaturesOfEntryInRange> searcher(this, creatureList, check);
    Cell::VisitObjects(this, searcher, maxSearchRange);
}

void WorldObject::GetCreatureListWithEntryInGrid(std::list<Creature*>& creatureList, std::vector<uint32> const& entries, float maxSearchRange) const
{
    Acore::AllCreaturesMatchingOneEntryInRange check(this, entries, maxSearchRange);
    Acore::CreatureListSearcher searcher(this, creatureList, check);
    Cell::VisitObjects(this, searcher, maxSearchRange);
}

void WorldObject::GetDeadCreatureListInGrid(std::list<Creature*>& creaturedeadList, float maxSearchRange, bool alive /*= false*/) const
{
    Acore::AllDeadCreaturesInRange check(this, maxSearchRange, alive);
    Acore::CreatureListSearcher<Acore::AllDeadCreaturesInRange> searcher(this, creaturedeadList, check);
    Cell::VisitObjects(this, searcher, maxSearchRange);
}

/*
namespace Acore
{
    class NearUsedPosDo
    {
        public:
            NearUsedPosDo(WorldObject const& obj, WorldObject const* searcher, float angle, ObjectPosSelector& selector)
                : i_object(obj), i_searcher(searcher), i_angle(angle), i_selector(selector) {}

            void operator()(Corpse*) const {}
            void operator()(DynamicObject*) const {}

            void operator()(Creature* c) const
            {
                // skip self or target
                if (c == i_searcher || c == &i_object)
                    return;

                float x, y, z;

                if (!c->IsAlive() || c->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED) ||
                    !c->GetMotionMaster()->GetDestination(x, y, z))
                {
                    x = c->GetPositionX();
                    y = c->GetPositionY();
                }

                add(c, x, y);
            }

            template<class T>
                void operator()(T* u) const
            {
                // skip self or target
                if (u == i_searcher || u == &i_object)
                    return;

                float x, y;

                x = u->GetPositionX();
                y = u->GetPositionY();

                add(u, x, y);
            }

            // we must add used pos that can fill places around center
            void add(WorldObject* u, float x, float y) const
            {
                // u is too nearest/far away to i_object
                if (!i_object.IsInRange2d(x, y, i_selector.m_dist - i_selector.m_size, i_selector.m_dist + i_selector.m_size))
                    return;

                float angle = i_object.GetAngle(u)-i_angle;

                // move angle to range -pi ... +pi
                while (angle > M_PI)
                    angle -= 2.0f * M_PI;
                while (angle < -M_PI)
                    angle += 2.0f * M_PI;

                // dist include size of u
                float dist2d = i_object.GetDistance2d(x, y);
                i_selector.AddUsedPos(u->GetObjectSize(), angle, dist2d + i_object.GetObjectSize());
            }
        private:
            WorldObject const& i_object;
            WorldObject const* i_searcher;
            float              i_angle;
            ObjectPosSelector& i_selector;
    };
}                                                           // namespace Acore
*/

//===================================================================================================

void WorldObject::GetNearPoint2D(WorldObject const* searcher, float& x, float& y, float distance2d, float absAngle, Position const* startPos) const
{
    float effectiveReach = GetCombatReach();

    if (searcher)
    {
        effectiveReach += searcher->GetCombatReach();

        if (this != searcher)
        {
            float myHover = 0.0f, searcherHover = 0.0f;
            if (Unit const* unit = ToUnit())
                myHover = unit->GetHoverHeight();
            if (Unit const* searchUnit = searcher->ToUnit())
                searcherHover = searchUnit->GetHoverHeight();

            float hoverDelta = myHover - searcherHover;
            if (hoverDelta != 0.0f)
                effectiveReach = std::sqrt(std::max(effectiveReach * effectiveReach - hoverDelta * hoverDelta, 0.0f));
        }
    }

    float positionX = startPos ? startPos->GetPositionX() : GetPositionX();
    float positionY = startPos ? startPos->GetPositionY() : GetPositionY();

    x = positionX + (effectiveReach + distance2d) * std::cos(absAngle);
    y = positionY + (effectiveReach + distance2d) * std::sin(absAngle);

    Acore::NormalizeMapCoord(x);
    Acore::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint2D(float& x, float& y, float distance2d, float absAngle, Position const* startPos) const
{
    GetNearPoint2D(nullptr, x, y, distance2d, absAngle, startPos);
}

void WorldObject::GetNearPoint(WorldObject const* searcher, float& x, float& y, float& z, float searcher_size, float distance2d, float absAngle, float controlZ, Position const* startPos) const
{
    GetNearPoint2D(x, y, distance2d + searcher_size, absAngle, startPos);
    z = GetPositionZ();

    if (searcher)
    {
        if (Unit const* unit = searcher->ToUnit(); Unit const* target = ToUnit())
        {
            if (unit && target && unit->IsInWater() && target->IsInWater())
            {
                // if the searcher is in water
                // we have no ground so we can
                // set the target height to the
                // z-coord to keep the searcher
                // at the correct height (face to face)
                z += GetCollisionHeight() - unit->GetCollisionHeight();
            }
        }
        searcher->UpdateAllowedPositionZ(x, y, z);
    }
    else
    {
        UpdateAllowedPositionZ(x, y, z);
    }

    // if detection disabled, return first point
    if (!sWorld->getBoolConfig(CONFIG_DETECT_POS_COLLISION))
        return;

    // return if the point is already in LoS
    if (!controlZ && IsWithinLOS(x, y, z))
        return;

    // remember first point
    float first_x = x;
    float first_y = y;
    float first_z = z;

    // loop in a circle to look for a point in LoS using small steps
    for (float angle = float(M_PI) / 8; angle < float(M_PI) * 2; angle += float(M_PI) / 8)
    {
        GetNearPoint2D(x, y, distance2d + searcher_size, absAngle + angle, startPos);
        z = GetPositionZ();
        UpdateAllowedPositionZ(x, y, z);
        if (controlZ && fabsf(GetPositionZ() - z) > controlZ)
            continue;

        if (IsWithinLOS(x, y, z))
            return;
    }

    // still not in LoS, give up and return first position found
    if (startPos)
    {
        x = searcher->GetPositionX();
        y = searcher->GetPositionY();
        z = searcher->GetPositionZ();
    }
    else
    {
        x = first_x;
        y = first_y;
        z = first_z;
    }
}

void WorldObject::GetVoidClosePoint(float& x, float& y, float& z, float size, float distance2d /*= 0*/, float relAngle /*= 0*/, float controlZ /*= 0*/) const
{
    // angle calculated from current orientation
    GetNearPoint(nullptr, x, y, z, size, distance2d, GetOrientation() + relAngle, controlZ);
}

bool WorldObject::GetClosePoint(float& x, float& y, float& z, float size, float distance2d, float angle, WorldObject const* forWho, bool force) const
{
    // angle calculated from current orientation
    GetNearPoint(forWho, x, y, z, size, distance2d, GetOrientation() + angle);

    if (std::fabs(this->GetPositionZ() - z) > 3.0f || !IsWithinLOS(x, y, z))
    {
        x = this->GetPositionX();
        y = this->GetPositionY();
        z = this->GetPositionZ();
        if (forWho)
            if (Unit const* u = forWho->ToUnit())
                u->UpdateAllowedPositionZ(x, y, z);
    }
    float maxDist = GetObjectSize() + size + distance2d + 1.0f;
    if (GetExactDistSq(x, y, z) >= maxDist * maxDist)
    {
        if (force)
        {
            x = this->GetPositionX();
            y = this->GetPositionY();
            z = this->GetPositionZ();
            return true;
        }
        return false;
    }
    return true;
}

Position WorldObject::GetNearPosition(float dist, float angle)
{
    Position pos = GetPosition();
    MovePosition(pos, dist, angle);
    return pos;
}

Position WorldObject::GetRandomNearPosition(float radius)
{
    Position pos = GetPosition();
    MovePosition(pos, radius * (float) rand_norm(), (float) rand_norm() * static_cast<float>(2 * M_PI));
    return pos;
}

void WorldObject::GetContactPoint(WorldObject const* obj, float& x, float& y, float& z, float distance2d) const
{
    // angle to face `obj` to `this` using distance includes size of `obj`
    GetNearPoint(obj, x, y, z, obj->GetObjectSize(), distance2d, GetAngle(obj));

    // Exclude gameobjects from LoS calculations
    if (std::fabs(this->GetPositionZ() - z) > 3.0f || (!IsGameObject() && !IsWithinLOS(x, y, z)))
    {
        x = this->GetPositionX();
        y = this->GetPositionY();
        z = this->GetPositionZ();
        obj->UpdateAllowedPositionZ(x, y, z);
    }
}

void WorldObject::GetChargeContactPoint(WorldObject const* obj, float& x, float& y, float& z, float distance2d) const
{
    // angle to face `obj` to `this` using distance includes size of `obj`
    GetNearPoint(obj, x, y, z, obj->GetObjectSize(), distance2d, GetAngle(obj));

    if (std::fabs(this->GetPositionZ() - z) > 3.0f || !IsWithinLOS(x, y, z))
    {
        x = this->GetPositionX();
        y = this->GetPositionY();
        z = this->GetPositionZ();
        obj->UpdateGroundPositionZ(x, y, z);
    }
}

[[nodiscard]] float WorldObject::GetObjectSize() const
{
    float combatReach = GetCombatReach();
    return combatReach > 0.0f ? combatReach : DEFAULT_WORLD_OBJECT_SIZE * GetObjectScale();
}

void WorldObject::MovePosition(Position& pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    destx = pos.m_positionX + dist * cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!Acore::IsValidMapCoord(destx, desty))
    {
        LOG_FATAL("entities.object", "WorldObject::MovePosition invalid coordinates X: {} and Y: {} were passed!", destx, desty);
        return;
    }

    ground = GetMapHeight(destx, desty, MAX_HEIGHT);
    floor = GetMapHeight(destx, desty, pos.m_positionZ);
    destz = std::fabs(ground - pos.m_positionZ) <= std::fabs(floor - pos.m_positionZ) ? ground : floor;

    float step = dist / 10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (std::fabs(pos.m_positionZ - destz) > 6.0f)
        {
            destx -= step * cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMapHeight(destx, desty, MAX_HEIGHT);
            floor = GetMapHeight(destx, desty, pos.m_positionZ);
            destz = std::fabs(ground - pos.m_positionZ) <= std::fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    Acore::NormalizeMapCoord(pos.m_positionX);
    Acore::NormalizeMapCoord(pos.m_positionY);
    UpdateGroundPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

Position WorldObject::GetFirstCollisionPosition(float startX, float startY, float startZ, float destX, float destY)
{
    auto dx = destX - startX;
    auto dy = destY - startY;

    auto ang = std::atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * M_PI + ang;
    Position pos = Position(startX, startY, startZ, ang);

    auto distance = pos.GetExactDist2d(destX,destY);

    MovePositionToFirstCollision(pos, distance, ang);
    return pos;
}

Position WorldObject::GetFirstCollisionPosition(float destX, float destY, float destZ)
{
    Position pos = GetPosition();
    auto distance = GetExactDistSq(destX,destY,destZ);

    auto dx = destX - pos.GetPositionX();
    auto dy = destY - pos.GetPositionY();

    auto ang = std::atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * M_PI + ang;

    MovePositionToFirstCollision(pos, distance, ang);
    return pos;
}

Position WorldObject::GetFirstCollisionPosition(float dist, float angle)
{
    Position pos = GetPosition();
    MovePositionToFirstCollision(pos, dist, angle);
    return pos;
}

void WorldObject::MovePositionToFirstCollision(Position& pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz;
    destx = pos.m_positionX + dist * cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);
    destz = pos.m_positionZ;

    if (!GetMap()->CheckCollisionAndGetValidCoords(this, pos.m_positionX, pos.m_positionY, pos.m_positionZ, destx, desty, destz, false))
        return;

    pos.SetOrientation(GetOrientation());
    pos.Relocate(destx, desty, destz);
}

void WorldObject::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    sScriptMgr->OnBeforeWorldObjectSetPhaseMask(this, m_phaseMask, newPhaseMask, m_useCombinedPhases, update);
    m_phaseMask = newPhaseMask;

    if (update && IsInWorld())
        UpdateObjectVisibility();
}

void WorldObject::PlayDistanceSound(uint32 sound_id, Player* target /*= nullptr*/)
{
    if (target)
        target->SendDirectMessage(WorldPackets::Misc::PlayObjectSound(GetGUID(), sound_id).Write());
    else
        SendMessageToSet(WorldPackets::Misc::PlayObjectSound(GetGUID(), sound_id).Write(), true);
}

void WorldObject::PlayDirectSound(uint32 sound_id, Player* target /*= nullptr*/)
{
    if (target)
        target->SendDirectMessage(WorldPackets::Misc::Playsound(sound_id).Write());
    else
        SendMessageToSet(WorldPackets::Misc::Playsound(sound_id).Write(), true);
}

void WorldObject::PlayRadiusSound(uint32 sound_id, float radius)
{
    std::vector<Player*> targets;
    Acore::AnyPlayerInObjectRangeCheck check(this, radius, false);
    Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(this, targets, check);
    Cell::VisitObjects(this, searcher, radius);

    for (Player* player : targets)
        player->SendDirectMessage(WorldPackets::Misc::Playsound(sound_id).Write());
}

void WorldObject::PlayDirectMusic(uint32 music_id, Player* target /*= nullptr*/)
{
    if (target)
        target->SendDirectMessage(WorldPackets::Misc::PlayMusic(music_id).Write());
    else
        SendMessageToSet(WorldPackets::Misc::PlayMusic(music_id).Write(), true);
}

void WorldObject::PlayRadiusMusic(uint32 music_id, float radius)
{
    std::vector<Player*> targets;
    Acore::AnyPlayerInObjectRangeCheck check(this, radius, false);
    Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(this, targets, check);
    Cell::VisitObjects(this, searcher, radius);

    for (Player* player : targets)
        player->SendDirectMessage(WorldPackets::Misc::PlayMusic(music_id).Write());
}

// Removes us from visibility for all players who are currently able to see us
void WorldObject::DestroyForVisiblePlayers()
{
    if (!IsInWorld())
        return;

    VisiblePlayersMap& visiblePlayerMap = GetObjectVisibilityContainer().GetVisiblePlayersMap();
    for (VisiblePlayersMap::iterator itr = visiblePlayerMap.begin(); itr != visiblePlayerMap.end();)
    {
        Player* player = itr->second;

        DestroyForPlayer(player);

        // Clean up visibility references now
        itr = GetObjectVisibilityContainer().UnlinkVisibilityFromWorldObject(player, itr);
    }
}

void WorldObject::UpdateObjectVisibility(bool /*forced*/, bool /*fromUpdate*/)
{
    //updates object's visibility for nearby players
    Acore::VisibleChangesNotifier notifier(*this);
    Cell::VisitObjects(this, notifier, GetVisibilityRange());
}

void WorldObject::AddToNotify(uint16 f)
{
    if (!(m_notifyflags & f))
        if (Unit* u = ToUnit())
        {
            if (f & NOTIFY_VISIBILITY_CHANGED)
            {
                uint32 EVENT_VISIBILITY_DELAY = u->FindMap() ? DynamicVisibilityMgr::GetVisibilityNotifyDelay(u->FindMap()->GetEntry()->InstanceType) : 1000;

                uint32 diff = getMSTimeDiff(u->m_last_notify_mstime, GameTime::GetGameTimeMS().count());
                if (diff >= EVENT_VISIBILITY_DELAY / 2)
                    EVENT_VISIBILITY_DELAY /= 2;
                else
                    EVENT_VISIBILITY_DELAY -= diff;
                u->m_delayed_unit_relocation_timer = EVENT_VISIBILITY_DELAY;
                u->m_last_notify_mstime = GameTime::GetGameTimeMS().count() + EVENT_VISIBILITY_DELAY - 1;
            }
            else if (f & NOTIFY_AI_RELOCATION)
            {
                u->m_delayed_unit_ai_notify_timer = u->FindMap() ? DynamicVisibilityMgr::GetAINotifyDelay(u->FindMap()->GetEntry()->InstanceType) : 500;
            }

            m_notifyflags |= f;
        }
}

void WorldObject::BuildUpdate(UpdateDataMapType& data_map)
{
    // Build update for self
    if (IsPlayer())
        BuildFieldsUpdate(ToPlayer(), data_map);

    // Build update for visible players
    DoForAllVisiblePlayers([this, &data_map](Player* player)
    {
        BuildFieldsUpdate(player, data_map);
    });

    ClearUpdateMask(false);
}

void WorldObject::GetCreaturesWithEntryInRange(std::list<Creature*>& creatureList, float radius, uint32 entry)
{
    Acore::AllCreaturesOfEntryInRange check(this, entry, radius);
    Acore::CreatureListSearcher<Acore::AllCreaturesOfEntryInRange> searcher(this, creatureList, check);
    Cell::VisitObjects(this, searcher, radius);
}

bool WorldObject::AddToObjectUpdate()
{
    GetMap()->AddUpdateObject(this);
    return true;
}

void WorldObject::RemoveFromObjectUpdate()
{
    GetMap()->RemoveUpdateObject(this);
}

ObjectGuid WorldObject::GetTransGUID() const
{
    if (GetTransport())
        return GetTransport()->GetGUID();

    return ObjectGuid::Empty;
}

float WorldObject::GetMapHeight(float x, float y, float z, bool vmap/* = true*/, float distanceToSearch/* = DEFAULT_HEIGHT_SEARCH*/) const
{
    if (z != MAX_HEIGHT)
        z += std::max(GetCollisionHeight(), Z_OFFSET_FIND_HEIGHT);

    return GetMap()->GetHeight(GetPhaseMask(), x, y, z, vmap, distanceToSearch);
}

float WorldObject::GetMapWaterOrGroundLevel(float x, float y, float z, float* ground/* = nullptr*/) const
{
    return GetMap()->GetWaterOrGroundLevel(GetPhaseMask(), x, y, z, ground,
        IsUnit() ? !static_cast<Unit const*>(this)->HasWaterWalkAura() : false,
        std::max(GetCollisionHeight(),  Z_OFFSET_FIND_HEIGHT));
}

float WorldObject::GetFloorZ() const
{
    if (_updatePositionData)
        const_cast<WorldObject*>(this)->UpdatePositionData();

    if (!IsInWorld())
        return _floorZ;

    return std::max<float>(_floorZ, GetMap()->GetGameObjectFloor(GetPhaseMask(), GetPositionX(), GetPositionY(), GetPositionZ() + std::max(GetCollisionHeight(), Z_OFFSET_FIND_HEIGHT)));
}

uint32 WorldObject::GetZoneId() const
{
    if (_updatePositionData)
        const_cast<WorldObject*>(this)->UpdatePositionData();

    return _zoneId;
}

uint32 WorldObject::GetAreaId() const
{
    if (_updatePositionData)
        const_cast<WorldObject*>(this)->UpdatePositionData();

    return _areaId;
}

void WorldObject::GetZoneAndAreaId(uint32& zoneid, uint32& areaid) const
{
    if (_updatePositionData)
        const_cast<WorldObject*>(this)->UpdatePositionData();

    zoneid = _zoneId;
    areaid = _areaId;
}

bool WorldObject::IsOutdoors() const
{
    if (_updatePositionData)
        const_cast<WorldObject*>(this)->UpdatePositionData();

    return _outdoors;
}

LiquidData const& WorldObject::GetLiquidData() const
{
    if (_updatePositionData)
        const_cast<WorldObject*>(this)->UpdatePositionData();

    return _liquidData;
}

void WorldObject::AddAllowedLooter(ObjectGuid guid)
{
    _allowedLooters.insert(guid);
}

void WorldObject::SetAllowedLooters(GuidUnorderedSet const looters)
{
    _allowedLooters = looters;
}

void WorldObject::ResetAllowedLooters()
{
    _allowedLooters.clear();
}

bool WorldObject::HasAllowedLooter(ObjectGuid guid) const
{
    if (_allowedLooters.empty())
    {
        return true;
    }

    return _allowedLooters.find(guid) != _allowedLooters.end();
}

GuidUnorderedSet const& WorldObject::GetAllowedLooters() const
{
    return _allowedLooters;
}

void WorldObject::RemoveAllowedLooter(ObjectGuid guid)
{
    _allowedLooters.erase(guid);
}

bool WorldObject::IsUpdateNeeded()
{
    if (isActiveObject())
        return true;

    return false;
}

bool WorldObject::CanBeAddedToMapUpdateList()
{
    switch (GetTypeId())
    {
    case TYPEID_UNIT:
        return IsCreature();
    case TYPEID_DYNAMICOBJECT:
    case TYPEID_GAMEOBJECT:
        return true;
    default:
        return false;
    }

    return false;
}
