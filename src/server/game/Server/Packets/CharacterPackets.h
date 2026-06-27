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

#ifndef CharacterPackets_h__
#define CharacterPackets_h__

#include "ObjectGuid.h"
#include "Optional.h"
#include "Packet.h"
#include "PacketUtilities.h"
#include "Position.h"
#include <array>
#include <vector>

namespace WorldPackets
{
    namespace Character
    {
        // 3.4.3 modern character customization choice (replaces legacy skin/face/hair fields)
        struct ChrCustomizationChoice
        {
            uint32 ChrCustomizationOptionID = 0;
            uint32 ChrCustomizationChoiceID = 0;
        };

        struct VisualItemInfo
        {
            uint32 DisplayID = 0;
            uint32 DisplayEnchantID = 0;
            int32 SecondaryItemModifiedAppearanceID = 0;
            uint8 InvType = 0;
            uint8 Subclass = 0;
        };

        struct RaceUnlock
        {
            int32 RaceID = 0;
            bool HasExpansion = false;
            bool HasAchievement = false;
            bool HasHeritageArmor = false;
            bool IsLocked = false;
        };

        struct UnlockedConditionalAppearance
        {
            int32 AchievementID = 0;
            int32 Unused = 0;
        };

        struct RaceLimitDisableInfo
        {
            int32 RaceID = 0;
            int32 BlockReason = 0;
        };

        class EnumCharactersResult final : public ServerPacket
        {
        public:
            EnumCharactersResult() : ServerPacket(SMSG_ENUM_CHARACTERS_RESULT, 64) { }

            WorldPacket const* Write() override;

            struct CharacterInfo
            {
                ObjectGuid Guid;
                uint64 GuildClubMemberID = 0;
                uint8 ListPosition = 0;
                uint8 RaceID = 0;
                uint8 ClassID = 0;
                uint8 SexID = 0;
                std::vector<ChrCustomizationChoice> Customizations;
                uint8 ExperienceLevel = 0;
                int32 ZoneID = 0;
                int32 MapID = 0;
                Position PreloadPos;
                ObjectGuid GuildGUID;
                uint32 Flags = 0;
                uint32 Flags2 = 0;
                uint32 Flags3 = 0;
                uint32 PetCreatureDisplayID = 0;
                uint32 PetExperienceLevel = 0;
                uint32 PetCreatureFamilyID = 0;
                std::array<uint32, 2> ProfessionIds = { };
                std::array<VisualItemInfo, 34> VisualItems = { };
                Timestamp<> LastPlayedTime;
                int16 SpecID = 0;
                int32 Unknown703 = 0;
                int32 LastLoginVersion = 0;
                uint32 Flags4 = 0;
                uint32 OverrideSelectScreenFileDataID = 0;
                std::string Name;
                bool FirstLogin = false;
                bool BoostInProgress = false;
                uint32 unkWod61x = 0;
                bool RpeResetAvailable = false;
                bool RpeResetQuestClearAvailable = false;

                friend ByteBuffer& operator<<(ByteBuffer& data, CharacterInfo const& charInfo);
            };

            bool Success = true;
            bool IsDeletedCharacters = false;
            bool IsNewPlayerRestrictionSkipped = false;
            bool IsNewPlayerRestricted = false;
            bool IsNewPlayer = false;
            bool IsTrialAccountRestricted = false;
            int32 MaxCharacterLevel = 0;
            Optional<uint32> DisabledClassesMask;
            std::vector<CharacterInfo> Characters;
            std::vector<RaceUnlock> RaceUnlockData;
            std::vector<UnlockedConditionalAppearance> UnlockedConditionalAppearances;
            std::vector<RaceLimitDisableInfo> RaceLimitDisables;
        };

        class ShowingCloak final : public ClientPacket
        {
        public:
            ShowingCloak(WorldPacket&& packet) : ClientPacket(CMSG_SHOWING_CLOAK, std::move(packet)) { }

            void Read() override;

            bool ShowCloak = false;
        };

        class ShowingHelm final : public ClientPacket
        {
        public:
            ShowingHelm(WorldPacket&& packet) : ClientPacket(CMSG_SHOWING_HELM, std::move(packet)) { }

            void Read() override;

            bool ShowHelm = false;
        };

        class LogoutRequest final : public ClientPacket
        {
        public:
            LogoutRequest(WorldPacket&& packet) : ClientPacket(std::move(packet)) { }

            void Read() override { }
        };

        class LogoutResponse final : public ServerPacket
        {
        public:
            LogoutResponse() : ServerPacket(SMSG_LOGOUT_RESPONSE, 4 + 1) { }

            WorldPacket const* Write() override;

            uint32 LogoutResult = 0;
            bool Instant = false;
        };

        class LogoutComplete final : public ServerPacket
        {
        public:
            LogoutComplete() : ServerPacket(SMSG_LOGOUT_COMPLETE, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class LogoutCancel final : public ClientPacket
        {
        public:
            LogoutCancel(WorldPacket&& packet) : ClientPacket(std::move(packet)) { }

            void Read() override { }
        };

        class LogoutCancelAck final : public ServerPacket
        {
        public:
            LogoutCancelAck() : ServerPacket(SMSG_LOGOUT_CANCEL_ACK, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class PlayerLogout final : public ClientPacket
        {
        public:
            PlayerLogout(WorldPacket&& packet) : ClientPacket(std::move(packet)) { }

            void Read() override { }
        };

        class PlayedTimeClient final : public ClientPacket
        {
        public:
            PlayedTimeClient(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_PLAYED_TIME, std::move(packet)) { }

            void Read() override;

            bool TriggerScriptEvent = false;
        };

        class PlayedTime final : public ServerPacket
        {
        public:
            PlayedTime() : ServerPacket(SMSG_PLAYED_TIME, 9) { }

            WorldPacket const* Write() override;

            uint32 TotalTime = 0;
            uint32 LevelTime = 0;
            bool TriggerScriptEvent = false;
        };
    }
}

#endif // CharacterPackets_h__
