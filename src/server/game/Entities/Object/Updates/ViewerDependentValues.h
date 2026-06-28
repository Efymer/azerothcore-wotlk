/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ViewerDependentValues_h__
#define ViewerDependentValues_h__

// [54261] Conversation.h include dropped (ConversationData/ConversationLine omitted)
#include "Creature.h"
#include "GameObject.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Trainer.h"
#include "World.h"
#include "WorldSession.h"

namespace UF
{
template<typename Tag>
class ViewerDependentValue
{
};

template<>
class ViewerDependentValue<UF::ObjectData::EntryIDTag>
{
public:
    using value_type = UF::ObjectData::EntryIDTag::value_type;

    static value_type GetValue(UF::ObjectData const* objectData, Object const* /*object*/, Player const* /*receiver*/)
    {
        // [1c.4] AC 3.3.5 has no creature-id-visible-to-summoner concept; always return the real EntryID.
        return objectData->EntryID;
    }
};

template<>
class ViewerDependentValue<UF::ObjectData::DynamicFlagsTag>
{
public:
    using value_type = UF::ObjectData::DynamicFlagsTag::value_type;

    static value_type GetValue(UF::ObjectData const* objectData, Object const* object, Player const* receiver)
    {
        value_type dynamicFlags = objectData->DynamicFlags;
        if (Unit const* unit = object->ToUnit())
        {
            dynamicFlags &= ~(UNIT_DYNFLAG_TAPPED | UNIT_DYNFLAG_TAPPED_BY_PLAYER);

            if (Creature const* creature = object->ToCreature())
            {
                if (creature->hasLootRecipient())
                {
                    dynamicFlags |= UNIT_DYNFLAG_TAPPED;
                    if (creature->isTappedBy(receiver))
                        dynamicFlags |= UNIT_DYNFLAG_TAPPED_BY_PLAYER;
                }

                if (dynamicFlags & UNIT_DYNFLAG_LOOTABLE && !const_cast<Player*>(receiver)->isAllowedToLoot(creature))
                    dynamicFlags &= ~UNIT_DYNFLAG_LOOTABLE;

                // [1c.4] TODO: UNIT_DYNFLAG_CAN_SKIN / Creature::IsSkinnedBy absent in AC 3.3.5 — skin masking dropped.
            }

            // unit UNIT_DYNFLAG_TRACK_UNIT should only be sent to caster of SPELL_AURA_MOD_STALKED auras
            if (dynamicFlags & UNIT_DYNFLAG_TRACK_UNIT)
                if (!unit->HasAuraTypeWithCaster(SPELL_AURA_MOD_STALKED, receiver->GetGUID()))
                    dynamicFlags &= ~UNIT_DYNFLAG_TRACK_UNIT;
        }
        else if (GameObject const* gameObject = object->ToGameObject())
        {
            uint16 dynFlags = 0;
            uint16 pathProgress = 0xFFFF;
            switch (gameObject->GetGoType())
            {
                case GAMEOBJECT_TYPE_QUESTGIVER:
                    if (gameObject->ActivateToQuest(const_cast<Player*>(receiver)))
                        dynFlags |= GO_DYNFLAG_LO_ACTIVATE;
                    break;
                case GAMEOBJECT_TYPE_CHEST:
                    if (gameObject->ActivateToQuest(const_cast<Player*>(receiver)))
                        dynFlags |= GO_DYNFLAG_LO_ACTIVATE | GO_DYNFLAG_LO_SPARKLE;
                    else if (receiver->IsGameMaster())
                        dynFlags |= GO_DYNFLAG_LO_ACTIVATE;
                    break;
                case GAMEOBJECT_TYPE_GOOBER:
                    if (gameObject->ActivateToQuest(const_cast<Player*>(receiver)))
                    {
                        if (gameObject->GetGoState() != GO_STATE_ACTIVE)
                            dynFlags |= GO_DYNFLAG_LO_ACTIVATE;
                    }
                    else if (receiver->IsGameMaster())
                        dynFlags |= GO_DYNFLAG_LO_ACTIVATE;
                    break;
                case GAMEOBJECT_TYPE_GENERIC:
                    if (gameObject->ActivateToQuest(const_cast<Player*>(receiver)))
                        dynFlags |= GO_DYNFLAG_LO_SPARKLE;
                    break;
                case GAMEOBJECT_TYPE_TRANSPORT:
                case GAMEOBJECT_TYPE_MO_TRANSPORT:
                {
                    dynFlags = dynamicFlags & 0xFFFF;
                    pathProgress = dynamicFlags >> 16;
                    break;
                }
                default:
                    break;
            }

            // [1c.4] TODO: GO_DYNFLAG_LO_HIGHLIGHT / GO_DYNFLAG_LO_DEPLETED, GAMEOBJECT_TYPE_GATHERING_NODE,
            // GameObject::CanInteractWithCapturePoint and GameObject::MeetsInteractCondition are absent in AC 3.3.5 — dropped.

            dynamicFlags = (uint32(pathProgress) << 16) | uint32(dynFlags);
        }

        return dynamicFlags;
    }
};

template<>
class ViewerDependentValue<UF::UnitData::DisplayIDTag>
{
public:
    using value_type = UF::UnitData::DisplayIDTag::value_type;

    static value_type GetValue(UF::UnitData const* unitData, Unit const* unit, Player const* receiver)
    {
        value_type displayId = unitData->DisplayID;
        if (unit->IsCreature())
        {
            CreatureTemplate const* cinfo = unit->ToCreature()->GetCreatureTemplate();

            // [1c.4] AC 3.3.5 has no creature-id/display-id-visible-to-summoner concept — summon spoof dropped.

            // this also applies for transform auras
            if (SpellInfo const* transform = sSpellMgr->GetSpellInfo(unit->getTransForm()))
            {
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    if (transform->Effects[i].IsAura(SPELL_AURA_TRANSFORM))
                    {
                        if (CreatureTemplate const* transformInfo = sObjectMgr->GetCreatureTemplate(transform->Effects[i].MiscValue))
                        {
                            cinfo = transformInfo;
                            break;
                        }
                    }
                }
            }

            if (cinfo->HasFlagsExtra(CREATURE_FLAG_EXTRA_TRIGGER))
                if (receiver->IsGameMaster())
                    displayId = cinfo->GetFirstVisibleModel()->CreatureDisplayID;
        }

        return displayId;
    }
};

template<>
class ViewerDependentValue<UF::UnitData::FactionTemplateTag>
{
public:
    using value_type = UF::UnitData::FactionTemplateTag::value_type;

    static value_type GetValue(UF::UnitData const* unitData, Unit const* unit, Player const* receiver)
    {
        value_type factionTemplate = unitData->FactionTemplate;
        if (unit->IsControlledByPlayer() && receiver != unit && sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP) && unit->IsInRaidWith(receiver))
        {
            FactionTemplateEntry const* ft1 = unit->GetFactionTemplateEntry();
            FactionTemplateEntry const* ft2 = receiver->GetFactionTemplateEntry();
            if (ft1 && ft2 && !ft1->IsFriendlyTo(*ft2))
                // pretend that all other HOSTILE players have own faction, to allow follow, heal, rezz (trade wont work)
                factionTemplate = receiver->GetFaction();
        }

        return factionTemplate;
    }
};

template<>
class ViewerDependentValue<UF::UnitData::FlagsTag>
{
public:
    using value_type = UF::UnitData::FlagsTag::value_type;

    static value_type GetValue(UF::UnitData const* unitData, Unit const* /*unit*/, Player const* receiver)
    {
        value_type flags = unitData->Flags;
        // [1c.4] UNIT_FLAG_UNINTERACTIBLE absent in AC 3.3.5 — mapped to UNIT_FLAG_NOT_SELECTABLE.
        // Gamemasters should be always able to interact with units - remove not-selectable flag
        if (receiver->IsGameMaster())
            flags &= ~UNIT_FLAG_NOT_SELECTABLE;

        return flags;
    }
};

template<>
class ViewerDependentValue<UF::UnitData::Flags3Tag>
{
public:
    using value_type = UF::UnitData::Flags3Tag::value_type;

    static value_type GetValue(UF::UnitData const* unitData, Unit const* /*unit*/, Player const* /*receiver*/)
    {
        // [1c.4] TODO: UNIT_FLAG3_ALREADY_SKINNED / Creature::IsSkinnedBy absent in AC 3.3.5 — skin masking dropped.
        return unitData->Flags3;
    }
};

template<>
class ViewerDependentValue<UF::UnitData::AuraStateTag>
{
public:
    using value_type = UF::UnitData::AuraStateTag::value_type;

    static value_type GetValue(UF::UnitData const* /*unitData*/, Unit const* unit, Player const* receiver)
    {
        // Check per caster aura states to not enable using a spell in client if specified aura is not by target
        return unit->BuildAuraStateUpdateForTarget(const_cast<Player*>(receiver));
    }
};

template<>
class ViewerDependentValue<UF::UnitData::PvpFlagsTag>
{
public:
    using value_type = UF::UnitData::PvpFlagsTag::value_type;

    static value_type GetValue(UF::UnitData const* unitData, Unit const* unit, Player const* receiver)
    {
        value_type pvpFlags = unitData->PvpFlags;
        if (unit->IsControlledByPlayer() && receiver != unit && sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP) && unit->IsInRaidWith(receiver))
        {
            FactionTemplateEntry const* ft1 = unit->GetFactionTemplateEntry();
            FactionTemplateEntry const* ft2 = receiver->GetFactionTemplateEntry();
            if (ft1 && ft2 && !ft1->IsFriendlyTo(*ft2))
                // Allow targeting opposite faction in party when enabled in config
                pvpFlags &= UNIT_BYTE2_FLAG_SANCTUARY;
        }

        return pvpFlags;
    }
};

template<>
class ViewerDependentValue<UF::UnitData::NpcFlagsTag>
{
public:
    using value_type = UF::UnitData::NpcFlagsTag::value_type;

    static value_type GetValue(UF::UnitData const* unitData, uint32 i, Unit const* unit, Player const* receiver)
    {
        value_type npcFlag = unitData->NpcFlags[i];
        if (i == 0 && unit->IsCreature())
        {
            if (!receiver->CanSeeSpellClickOn(unit->ToCreature()))
                npcFlag &= ~UNIT_NPC_FLAG_SPELLCLICK;

            // alistar: don't show training icon for non class trainers
            // [1c.4] AC has no CreatureTemplate::trainer_class; the trainer's class/race/spell requirement lives in
            // the `trainer` table. Trainer::IsTrainerValidForPlayer() is the correct AC equivalent of the old
            // class-trainer filter (and also covers race/spell trainers).
            if (unit->IsTrainer())
                if (Trainer::Trainer const* trainer = sObjectMgr->GetTrainer(unit->GetEntry()))
                    if (!trainer->IsTrainerValidForPlayer(receiver))
                        npcFlag &= ~(UNIT_NPC_FLAG_TRAINER_CLASS | UNIT_NPC_FLAG_TRAINER);
        }

        return npcFlag;
    }
};

template<>
class ViewerDependentValue<UF::GameObjectData::FlagsTag>
{
public:
    using value_type = UF::GameObjectData::FlagsTag::value_type;

    static value_type GetValue(UF::GameObjectData const* gameObjectData, GameObject const* gameObject, Player const* receiver)
    {
        value_type flags = gameObjectData->Flags;
        if (gameObject->GetGoType() == GAMEOBJECT_TYPE_CHEST)
            if (gameObject->GetGOInfo()->chest.groupLootRules && !gameObject->IsLootAllowedFor(receiver))
                flags |= GO_FLAG_LOCKED | GO_FLAG_NOT_SELECTABLE;

        return flags;
    }
};

template<>
class ViewerDependentValue<UF::GameObjectData::StateTag>
{
public:
    using value_type = UF::GameObjectData::StateTag::value_type;

    static value_type GetValue(UF::GameObjectData const* /*gameObjectData*/, GameObject const* gameObject, Player const* /*receiver*/)
    {
        // [1c.4] AC 3.3.5 has no per-viewer GO state (GetGoStateFor) — return the shared GO state.
        return gameObject->GetGoState();
    }
};

// [54261] ConversationData::LastLineEndTimeTag and ConversationLine::StartTimeTag
// viewer-dependent specializations omitted (no AC Conversation entity).
}

#endif // ViewerDependentValues_h__
