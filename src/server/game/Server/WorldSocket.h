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

#ifndef __WORLDSOCKET_H__
#define __WORLDSOCKET_H__

#include "Common.h"
#include "MPSCQueue.h"
#include "Socket.h"
#include "Util.h"
#include "WorldPacket.h"
#include "WorldPacketCrypt.h"
#include "WorldSession.h"
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <mutex>

using boost::asio::ip::tcp;

typedef struct z_stream_s z_stream;

// brick E1: migrated to the modern Acore::Net stack with the 3.4.3 V2 banner exchange and
// AES-256-GCM packet framing. The auth-handshake crypto (digest check + session/encrypt key
// derivation) is stubbed for brick E2 - see HandleAuthSession.
class EncryptablePacket : public WorldPacket
{
public:
    EncryptablePacket(WorldPacket const& packet, bool encrypt) : WorldPacket(packet), _encrypt(encrypt)
    {
        SocketQueueLink.store(nullptr, std::memory_order_relaxed);
    }

    bool NeedsEncryption() const { return _encrypt; }

    std::atomic<EncryptablePacket*> SocketQueueLink;

private:
    bool _encrypt;
};

namespace JSON::RealmList
{
    class RealmJoinTicket;
}

namespace WorldPackets
{
    class ServerPacket;
    namespace Auth
    {
        class AuthSession;
        class AuthContinuedSession;
    }
}

#pragma pack(push, 1)

// 3.4.3 modern wire header (outbound, 16 bytes). Size is the encrypted payload length
// ({opcode||data}); Tag is the AES-256-GCM authentication tag.
struct PacketHeader
{
    uint32 Size;
    uint8 Tag[12];

    bool IsValidSize() const { return Size < 0x10000; }
};

// 3.4.3 modern wire header (inbound, 20 bytes). EncryptedOpcode is the first 4 bytes of the
// encrypted payload, peeked forward so the body decrypt stays contiguous.
struct IncomingPacketHeader : PacketHeader
{
    uint32 EncryptedOpcode;
};

#pragma pack(pop)

class AC_GAME_API WorldSocket final : public Acore::Net::Socket<>
{
    static uint32 const MinSizeForCompression;

    // 3.4.3 world-auth seeds. These MUST stay byte-exact with the client (and the bnetserver) or
    // the digest/session-key derivation produces a mismatch and the client rejects the connection.
    static std::array<uint8, 32> const AuthCheckSeed;
    static std::array<uint8, 32> const SessionKeySeed;
    static std::array<uint8, 32> const ContinuedSessionSeed;
    static std::array<uint8, 32> const EncryptionKeySeed;

    using BaseSocket = Acore::Net::Socket<>;

public:
    WorldSocket(Acore::Net::IoContextTcpSocket&& socket);
    ~WorldSocket();

    WorldSocket(WorldSocket const& right) = delete;
    WorldSocket& operator=(WorldSocket const& right) = delete;

    void Start() override;
    bool Update() override;

    void SendPacket(WorldPacket const& packet);

    ConnectionType GetConnectionType() const { return _type; }

    void SetSendBufferSize(std::size_t sendBufferSize) { _sendBufferSize = sendBufferSize; }

    bool IsLoggingPackets() const { return _loggingPackets; }
    void SetPacketLogging(bool state) { _loggingPackets = state; }

    // 3.4.3: lets a WorldSession adopt this socket as its second (instance) connection.
    void SetWorldSession(WorldSession* session);
    // public so WorldSession::AddInstanceConnection can reject a bad instance handshake
    void SendAuthResponseError(uint32 code);

    void OnClose() override;
    Acore::Net::SocketReadCallbackResult ReadHandler() override;

    void QueueQuery(QueryCallback&& queryCallback);

    // V2 banner / handshake setup (called from the connection-initializer chain)
    void SendAuthSession();
    bool InitializeCompression();

protected:
    bool ReadHeaderHandler();

    enum class ReadDataHandlerResult
    {
        Ok = 0,
        Error = 1,
        WaitingForQuery = 2
    };

    ReadDataHandlerResult ReadDataHandler();

private:
    /// writes network.opcode log
    /// accessing WorldSession is not threadsafe, only do it when holding _worldSessionLock
    void LogOpcodeText(OpcodeClient opcode, std::unique_lock<std::mutex> const& guard) const;

    /// sends and logs network.opcode without accessing WorldSession
    void SendPacketAndLogOpcode(WorldPacket const& packet);
    void WritePacketToBuffer(EncryptablePacket const& packet, MessageBuffer& buffer);
    uint32 CompressPacket(uint8* buffer, WorldPacket const& packet);

    void HandleAuthSession(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession);
    void HandleAuthSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession,
        std::shared_ptr<JSON::RealmList::RealmJoinTicket> joinTicket, PreparedQueryResult result);
    void LoadSessionPermissionsCallback(PreparedQueryResult result);
    void HandleAuthContinuedSession(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession);
    void HandleAuthContinuedSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession, PreparedQueryResult result);
    void HandleConnectToFailed(WorldPacket& recvPacket);
    void HandleEnterEncryptedModeAck();

    bool HandlePing(WorldPacket& recvPacket);

    ConnectionType _type;
    uint64 _key;

    std::array<uint8, 32> _serverChallenge;
    WorldPacketCrypt _authCrypt;
    SessionKey _sessionKey;
    std::array<uint8, 32> _encryptKey;

    TimePoint _LastPingTime;
    uint32 _OverSpeedPings;

    std::mutex _worldSessionLock;
    WorldSession* _worldSession;
    bool _authed;
    bool _canRequestHotfixes;

    MessageBuffer _headerBuffer;
    MessageBuffer _packetBuffer;
    MPSCQueue<EncryptablePacket, &EncryptablePacket::SocketQueueLink> _bufferQueue;
    std::size_t _sendBufferSize;

    z_stream* _compressionStream;

    QueryCallbackProcessor _queryProcessor;
    std::string _ipCountry;

    bool _loggingPackets;
};

#endif
