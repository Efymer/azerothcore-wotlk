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

#include "CharacterPackets.h"

void WorldPackets::Character::ShowingCloak::Read()
{
    _worldPacket >> ShowCloak;
}

void WorldPackets::Character::ShowingHelm::Read()
{
    _worldPacket >> ShowHelm;
}

WorldPacket const* WorldPackets::Character::LogoutResponse::Write()
{
    _worldPacket << uint32(LogoutResult);
    _worldPacket << uint8(Instant);
    return &_worldPacket;
}

void WorldPackets::Character::PlayedTimeClient::Read()
{
    _worldPacket >> TriggerScriptEvent;
}

WorldPacket const* WorldPackets::Character::PlayedTime::Write()
{
    _worldPacket << uint32(TotalTime);
    _worldPacket << uint32(LevelTime);
    _worldPacket << uint8(TriggerScriptEvent);

    return &_worldPacket;
}

ByteBuffer& WorldPackets::Character::operator<<(ByteBuffer& data, EnumCharactersResult::CharacterInfo const& charInfo)
{
    data << charInfo.Guid;
    data << uint64(charInfo.GuildClubMemberID);
    data << uint8(charInfo.ListPosition);
    data << uint8(charInfo.RaceID);
    data << uint8(charInfo.ClassID);
    data << uint8(charInfo.SexID);
    data << uint32(charInfo.Customizations.size());
    data << uint8(charInfo.ExperienceLevel);
    data << int32(charInfo.ZoneID);
    data << int32(charInfo.MapID);
    data << float(charInfo.PreloadPos.GetPositionX());
    data << float(charInfo.PreloadPos.GetPositionY());
    data << float(charInfo.PreloadPos.GetPositionZ());
    data << charInfo.GuildGUID;
    data << uint32(charInfo.Flags);
    data << uint32(charInfo.Flags2);
    data << uint32(charInfo.Flags3);
    data << uint32(charInfo.PetCreatureDisplayID);
    data << uint32(charInfo.PetExperienceLevel);
    data << uint32(charInfo.PetCreatureFamilyID);
    data << uint32(charInfo.ProfessionIds[0]);
    data << uint32(charInfo.ProfessionIds[1]);

    for (VisualItemInfo const& visualItem : charInfo.VisualItems)
    {
        data << uint32(visualItem.DisplayID);
        data << uint32(visualItem.DisplayEnchantID);
        data << int32(visualItem.SecondaryItemModifiedAppearanceID);
        data << uint8(visualItem.InvType);
        data << uint8(visualItem.Subclass);
    }

    data << charInfo.LastPlayedTime;
    data << int16(charInfo.SpecID);
    data << int32(charInfo.Unknown703);
    data << int32(charInfo.LastLoginVersion);
    data << uint32(charInfo.Flags4);
    data << uint32(0); // MailSenders.size()
    data << uint32(0); // MailSenderTypes.size()
    data << uint32(charInfo.OverrideSelectScreenFileDataID);

    for (ChrCustomizationChoice const& customization : charInfo.Customizations)
    {
        data << uint32(customization.ChrCustomizationOptionID);
        data << uint32(customization.ChrCustomizationChoiceID);
    }

    data.WriteBits(charInfo.Name.length(), 6);
    data.WriteBit(charInfo.FirstLogin);
    data.WriteBit(charInfo.BoostInProgress);
    data.WriteBits(charInfo.unkWod61x, 5);
    data.WriteBits(0, 2);
    data.WriteBit(charInfo.RpeResetAvailable);
    data.WriteBit(charInfo.RpeResetQuestClearAvailable);
    data.FlushBits();

    // 3.4.3 packs the name length above (6 bits) then writes the raw bytes with no NUL terminator
    data.WriteString(charInfo.Name);

    return data;
}

WorldPacket const* WorldPackets::Character::EnumCharactersResult::Write()
{
    _worldPacket.WriteBit(Success);
    _worldPacket.WriteBit(IsDeletedCharacters);
    _worldPacket.WriteBit(IsNewPlayerRestrictionSkipped);
    _worldPacket.WriteBit(IsNewPlayerRestricted);
    _worldPacket.WriteBit(IsNewPlayer);
    _worldPacket.WriteBit(IsTrialAccountRestricted);
    _worldPacket.WriteBit(DisabledClassesMask.has_value());
    _worldPacket.FlushBits();

    _worldPacket << uint32(Characters.size());
    _worldPacket << int32(MaxCharacterLevel);
    _worldPacket << uint32(RaceUnlockData.size());
    _worldPacket << uint32(UnlockedConditionalAppearances.size());
    _worldPacket << uint32(RaceLimitDisables.size());

    if (DisabledClassesMask)
        _worldPacket << uint32(*DisabledClassesMask);

    for (UnlockedConditionalAppearance const& unlockedConditionalAppearance : UnlockedConditionalAppearances)
    {
        _worldPacket << int32(unlockedConditionalAppearance.AchievementID);
        _worldPacket << int32(unlockedConditionalAppearance.Unused);
    }

    for (RaceLimitDisableInfo const& raceLimitDisableInfo : RaceLimitDisables)
    {
        _worldPacket << int32(raceLimitDisableInfo.RaceID);
        _worldPacket << int32(raceLimitDisableInfo.BlockReason);
    }

    for (CharacterInfo const& charInfo : Characters)
        _worldPacket << charInfo;

    for (RaceUnlock const& raceUnlock : RaceUnlockData)
    {
        _worldPacket << int32(raceUnlock.RaceID);
        _worldPacket.WriteBit(raceUnlock.HasExpansion);
        _worldPacket.WriteBit(raceUnlock.HasAchievement);
        _worldPacket.WriteBit(raceUnlock.HasHeritageArmor);
        _worldPacket.WriteBit(raceUnlock.IsLocked);
        _worldPacket.FlushBits();
    }

    return &_worldPacket;
}
