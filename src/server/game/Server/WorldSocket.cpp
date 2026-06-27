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

#include "WorldSocket.h"
#include "AccountMgr.h"
#include "AuthenticationPackets.h"
#include "ClientBuildInfo.h"
#include "Config.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "HMAC.h"
#include "IPLocation.h"
#include "IpBanCheckConnectionInitializer.h"
#include "Opcodes.h"
#include "PacketLog.h"
#include "ProtobufJSON.h"
#include "Random.h"
#include "Realm.h"
#include "RealmList.pb.h"
#include "ScriptMgr.h"
#include "SessionKeyGenerator.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include "RBAC.h"
#include "zlib.h"
#include <algorithm>
#include <memory>

using boost::asio::ip::tcp;

#pragma pack(push, 1)

// Per-packet metadata that precedes a compressed payload inside a SMSG_COMPRESSED_PACKET.
struct CompressedWorldPacket
{
    uint32 UncompressedSize;
    uint32 UncompressedAdler;
    uint32 CompressedAdler;
};

#pragma pack(pop)

uint32 const WorldSocket::MinSizeForCompression = 0x400;

// 3.4.3.54261 world-auth seeds (16-byte), byte-exact with the 54261 client (HermesProxy WorldSocket.cs).
// These are part of the client/server contract and must not change.
std::array<uint8, 16> const WorldSocket::AuthCheckSeed = { 0xC5, 0xC6, 0x98, 0x95, 0x76, 0x3F, 0x1D, 0xCD, 0xB6, 0xA1, 0x37, 0x28, 0xB3, 0x12, 0xFF, 0x8A };
std::array<uint8, 16> const WorldSocket::SessionKeySeed = { 0x58, 0xCB, 0xCF, 0x40, 0xFE, 0x2E, 0xCE, 0xA6, 0x5A, 0x90, 0xB8, 0x01, 0x68, 0x6C, 0x28, 0x0B };
std::array<uint8, 16> const WorldSocket::ContinuedSessionSeed = { 0x16, 0xAD, 0x0C, 0xD4, 0x46, 0xF9, 0x4F, 0xB2, 0xEF, 0x7D, 0xEA, 0x2A, 0x17, 0x66, 0x4D, 0x2F };
std::array<uint8, 16> const WorldSocket::EncryptionKeySeed = { 0xE9, 0x75, 0x3C, 0x50, 0x90, 0x93, 0x61, 0xDA, 0x3B, 0x07, 0xEE, 0xFA, 0xFF, 0x9D, 0x41, 0xB8 };

WorldSocket::WorldSocket(Acore::Net::IoContextTcpSocket&& socket) : BaseSocket(std::move(socket)),
    _type(CONNECTION_TYPE_REALM), _key(0), _serverChallenge(), _sessionKey(), _encryptKey(), _OverSpeedPings(0),
    _worldSession(nullptr), _authed(false), _canRequestHotfixes(true), _headerBuffer(sizeof(IncomingPacketHeader)),
    _sendBufferSize(4096), _compressionStream(nullptr), _loggingPackets(false)
{
}

WorldSocket::~WorldSocket()
{
    if (_compressionStream)
    {
        deflateEnd(_compressionStream);
        delete _compressionStream;
    }
}

// 3.4.3 V2 banner exchange: the server sends its banner immediately, then reads and validates
// the client banner before the steady-state ReadConnectionInitializer takes over.
struct WorldSocketProtocolInitializer final : Acore::Net::SocketConnectionInitializer
{
    static constexpr std::string_view ServerConnectionInitialize = "WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT - V2\n";
    static constexpr std::string_view ClientConnectionInitialize = "WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER - V2\n";

    explicit WorldSocketProtocolInitializer(WorldSocket* socket) : _socket(socket) { }

    void Start() override
    {
        _packetBuffer.Resize(ClientConnectionInitialize.length());

        AsyncRead();

        MessageBuffer initializer;
        initializer.Write(ServerConnectionInitialize.data(), ServerConnectionInitialize.length());

        // - IoContext.run thread, safe.
        _socket->QueuePacket(std::move(initializer));
    }

    void AsyncRead()
    {
        _socket->AsyncRead(
            [socketRef = _socket->weak_from_this(), self = std::static_pointer_cast<WorldSocketProtocolInitializer>(this->shared_from_this())]
            {
                if (!socketRef.expired())
                    return self->ReadHandler();

                return Acore::Net::SocketReadCallbackResult::Stop;
            });
    }

    Acore::Net::SocketReadCallbackResult ReadHandler();

    void HandleDataReady();

private:
    WorldSocket* _socket;
    MessageBuffer _packetBuffer;
};

void WorldSocket::Start()
{
    // build initializer chain: IP ban check -> V2 banner exchange -> steady-state read
    std::array<std::shared_ptr<Acore::Net::SocketConnectionInitializer>, 3> initializers =
    { {
        std::make_shared<Acore::Net::IpBanCheckConnectionInitializer<WorldSocket>>(this),
        std::make_shared<WorldSocketProtocolInitializer>(this),
        std::make_shared<Acore::Net::ReadConnectionInitializer<WorldSocket>>(this),
    } };

    Acore::Net::SocketConnectionInitializer::SetupChain(initializers)->Start();
}

Acore::Net::SocketReadCallbackResult WorldSocketProtocolInitializer::ReadHandler()
{
    MessageBuffer& packet = _socket->GetReadBuffer();
    if (packet.GetActiveSize() > 0 && _packetBuffer.GetRemainingSpace() > 0)
    {
        // need to receive the banner
        std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
        _packetBuffer.Write(packet.GetReadPointer(), readHeaderSize);
        packet.ReadCompleted(readHeaderSize);

        if (_packetBuffer.GetRemainingSpace() == 0)
        {
            HandleDataReady();
            return Acore::Net::SocketReadCallbackResult::Stop;
        }

        // Couldn't receive the whole banner this time.
        ASSERT(packet.GetActiveSize() == 0);
    }

    return Acore::Net::SocketReadCallbackResult::KeepReading;
}

void WorldSocketProtocolInitializer::HandleDataReady()
{
    try
    {
        ByteBuffer buffer(std::move(_packetBuffer));
        if (buffer.ReadString(ClientConnectionInitialize.length()) != ClientConnectionInitialize)
        {
            _socket->CloseSocket();
            return;
        }
    }
    catch (ByteBufferException const& ex)
    {
        LOG_ERROR("network", "WorldSocket::InitializeHandler ByteBufferException {} occured while parsing initial packet from {}",
            ex.what(), _socket->GetRemoteIpAddress().to_string());
        _socket->CloseSocket();
        return;
    }

    if (!_socket->InitializeCompression())
        return;

    _socket->SendAuthSession();
    if (next)
        next->Start();
}

bool WorldSocket::InitializeCompression()
{
    _compressionStream = new z_stream();
    _compressionStream->zalloc = (alloc_func)nullptr;
    _compressionStream->zfree = (free_func)nullptr;
    _compressionStream->opaque = (voidpf)nullptr;
    _compressionStream->avail_in = 0;
    _compressionStream->next_in = nullptr;
    int32 z_res = deflateInit2(_compressionStream, sWorld->getIntConfig(CONFIG_COMPRESSION), Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (z_res != Z_OK)
    {
        CloseSocket();
        LOG_ERROR("network", "Can't initialize packet compression (zlib: deflateInit) Error code: {} ({})", z_res, zError(z_res));
        return false;
    }

    return true;
}

bool WorldSocket::Update()
{
    EncryptablePacket* queued;
    MessageBuffer buffer(_sendBufferSize);
    while (_bufferQueue.Dequeue(queued))
    {
        uint32 packetSize = queued->size() + 2 /*uint16 opcode*/;
        if (packetSize > MinSizeForCompression && queued->NeedsEncryption())
            packetSize = deflateBound(_compressionStream, packetSize) + sizeof(CompressedWorldPacket);

        // Flush current buffer if too small for next packet
        if (buffer.GetRemainingSpace() < packetSize + sizeof(PacketHeader))
        {
            QueuePacket(std::move(buffer));
            buffer.Resize(_sendBufferSize);
        }

        if (buffer.GetRemainingSpace() >= packetSize + sizeof(PacketHeader))
            WritePacketToBuffer(*queued, buffer);
        else    // single packet larger than _sendBufferSize
        {
            MessageBuffer packetBuffer(packetSize + sizeof(PacketHeader));
            WritePacketToBuffer(*queued, packetBuffer);
            QueuePacket(std::move(packetBuffer));
        }

        delete queued;
    }

    if (buffer.GetActiveSize() > 0)
        QueuePacket(std::move(buffer));

    if (!BaseSocket::Update())
        return false;

    _queryProcessor.ProcessReadyCallbacks();

    return true;
}

void WorldSocket::SendAuthSession()
{
    _serverChallenge = Acore::Crypto::GetRandomBytes<16>();

    WorldPackets::Auth::AuthChallenge challenge;
    challenge.Challenge = _serverChallenge;
    std::array<uint8, 32> dosChallenge = Acore::Crypto::GetRandomBytes<32>();
    memcpy(challenge.DosChallenge.data(), dosChallenge.data(), dosChallenge.size());
    challenge.DosZeroBits = 1;

    SendPacketAndLogOpcode(*challenge.Write());
}

void WorldSocket::OnClose()
{
    {
        std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
        _worldSession = nullptr;
    }
}

Acore::Net::SocketReadCallbackResult WorldSocket::ReadHandler()
{
    if (!IsOpen())
        return Acore::Net::SocketReadCallbackResult::Stop;

    MessageBuffer& packet = GetReadBuffer();
    while (packet.GetActiveSize() > 0)
    {
        if (_headerBuffer.GetRemainingSpace() > 0)
        {
            // need to receive the header
            std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _headerBuffer.GetRemainingSpace());
            _headerBuffer.Write(packet.GetReadPointer(), readHeaderSize);
            packet.ReadCompleted(readHeaderSize);

            if (_headerBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole header this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }

            // We just received nice new header
            if (!ReadHeaderHandler())
            {
                CloseSocket();
                return Acore::Net::SocketReadCallbackResult::Stop;
            }
        }

        // We have full read header, now check the data payload
        if (_packetBuffer.GetRemainingSpace() > 0)
        {
            // need more data in the payload
            std::size_t readDataSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
            _packetBuffer.Write(packet.GetReadPointer(), readDataSize);
            packet.ReadCompleted(readDataSize);

            if (_packetBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole data this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }
        }

        // just received fresh new payload
        ReadDataHandlerResult result = ReadDataHandler();
        _headerBuffer.Reset();
        if (result != ReadDataHandlerResult::Ok)
        {
            if (result != ReadDataHandlerResult::WaitingForQuery)
                CloseSocket();

            return Acore::Net::SocketReadCallbackResult::Stop;
        }
    }

    return Acore::Net::SocketReadCallbackResult::KeepReading;
}

void WorldSocket::QueueQuery(QueryCallback&& queryCallback)
{
    _queryProcessor.AddCallback(std::move(queryCallback));
}

void WorldSocket::SetWorldSession(WorldSession* session)
{
    std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
    _worldSession = session;
    _authed = true;
}

bool WorldSocket::ReadHeaderHandler()
{
    ASSERT(_headerBuffer.GetActiveSize() == sizeof(IncomingPacketHeader));

    IncomingPacketHeader* header = reinterpret_cast<IncomingPacketHeader*>(_headerBuffer.GetReadPointer());
    uint16 encryptedOpcode = header->EncryptedOpcode;

    if (!header->IsValidSize())
    {
        _authCrypt.PeekDecryptRecv(reinterpret_cast<uint8*>(&header->EncryptedOpcode), sizeof(encryptedOpcode));

        // CMSG_HOTFIX_REQUEST can be much larger than normal packets, allow receiving it once per session
        if (header->EncryptedOpcode != CMSG_HOTFIX_REQUEST || header->Size > 0x100000 || !_canRequestHotfixes)
        {
            LOG_ERROR("network", "WorldSocket::ReadHeaderHandler(): client {} sent malformed packet (size: {}, opcode {})",
                GetRemoteIpAddress().to_string(), header->Size, uint32(header->EncryptedOpcode));
            return false;
        }
    }

    _packetBuffer.Resize(header->Size);
    _packetBuffer.Write(&encryptedOpcode, sizeof(encryptedOpcode));
    return true;
}

struct AccountInfo
{
    uint32 Id;
    std::array<uint8, 64> KeyData;      // 3.4.3: the 64-byte bnet key blob (session_key_bnet) the world key derives from
    std::string LastIP;
    bool IsLockedToIP;
    std::string LockCountry;
    uint8 Expansion;
    uint32 Flags;
    int64 MuteTime;
    uint32 Build;
    LocaleConstant Locale;
    uint32 Recruiter;
    std::string OS;
    Minutes TimezoneOffset;
    AccountTypes Security;
    bool IsBanned;
    bool IsRectuiter;

    explicit AccountInfo(Field* fields)
    {
        // LOGIN_SEL_ACCOUNT_INFO_FOR_WORLD_AUTH columns:
        //           0                   1            2          3               4            5         6           7                8           9             10        11
        // SELECT a.id, a.session_key_bnet, a.last_ip, a.locked, a.lock_country, a.expansion, a.Flags, a.mutetime, a.client_build, a.locale, a.recruiter, a.os,
        //                       12             13                                                             14    15
        // a.timezone_offset, aa.gmlevel, ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, r.id
        Id = fields[0].Get<uint32>();
        KeyData = fields[1].Get<Binary, 64>();
        LastIP = fields[2].Get<std::string>();
        IsLockedToIP = fields[3].Get<bool>();
        LockCountry = fields[4].Get<std::string>();
        Expansion = fields[5].Get<uint8>();
        Flags = fields[6].Get<uint32>();
        MuteTime = fields[7].Get<int64>();
        Build = fields[8].Get<uint32>();
        Locale = LocaleConstant(fields[9].Get<uint8>());
        Recruiter = fields[10].Get<uint32>();
        OS = fields[11].Get<std::string>();
        TimezoneOffset = Minutes(fields[12].Get<int16>());
        Security = AccountTypes(fields[13].Get<uint8>());
        IsBanned = fields[14].Get<uint64>() != 0;
        IsRectuiter = fields[15].Get<uint32>() != 0;

        uint32 world_expansion = sWorld->getIntConfig(CONFIG_EXPANSION);
        if (Expansion > world_expansion)
            Expansion = world_expansion;

        if (Locale >= TOTAL_LOCALES)
            Locale = LOCALE_enUS;
    }
};

WorldSocket::ReadDataHandlerResult WorldSocket::ReadDataHandler()
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(_headerBuffer.GetReadPointer());

    if (!_authCrypt.DecryptRecv(_packetBuffer.GetReadPointer(), header->Size, header->Tag))
    {
        LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} failed to decrypt packet (size: {})",
            GetRemoteIpAddress().to_string(), header->Size);
        return ReadDataHandlerResult::Error;
    }

    WorldPacket packet(0, std::move(_packetBuffer), GetConnectionType());
    // 3.4.3.54261 wire opcodes are FLAT uint16 values - read 2 bytes then widen to the enum
    OpcodeClient opcode = static_cast<OpcodeClient>(packet.read<uint16>());
    if (!opcodeTable.IsValid(opcode))
    {
        LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent wrong opcode (opcode: {})",
            GetRemoteIpAddress().to_string(), uint32(opcode));
        return ReadDataHandlerResult::Error;
    }

    packet.SetOpcode(opcode);

    if (sPacketLog->CanLogPacket() && IsLoggingPackets())
        sPacketLog->LogPacket(packet, CLIENT_TO_SERVER, GetRemoteIpAddress(), GetRemotePort());

    std::unique_lock<std::mutex> sessionGuard(_worldSessionLock, std::defer_lock);

    switch (opcode)
    {
        case CMSG_PING:
        {
            LogOpcodeText(opcode, sessionGuard);
            try
            {
                return HandlePing(packet) ? ReadDataHandlerResult::Ok : ReadDataHandlerResult::Error;
            }
            catch (ByteBufferException const&)
            {
            }
            LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_PING", GetRemoteIpAddress().to_string());
            return ReadDataHandlerResult::Error;
        }
        case CMSG_AUTH_SESSION:
        {
            LogOpcodeText(opcode, sessionGuard);
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    LOG_ERROR("network", "WorldSocket::ProcessIncoming: received duplicate CMSG_AUTH_SESSION from {}", _worldSession->GetPlayerInfo());
                return ReadDataHandlerResult::Error;
            }

            std::shared_ptr<WorldPackets::Auth::AuthSession> authSession = std::make_shared<WorldPackets::Auth::AuthSession>(std::move(packet));
            if (!authSession->ReadNoThrow())
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_SESSION", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }
            HandleAuthSession(authSession);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        case CMSG_AUTH_CONTINUED_SESSION:
        {
            LogOpcodeText(opcode, sessionGuard);
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    LOG_ERROR("network", "WorldSocket::ProcessIncoming: received duplicate CMSG_AUTH_CONTINUED_SESSION from {}", _worldSession->GetPlayerInfo());
                return ReadDataHandlerResult::Error;
            }

            std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession = std::make_shared<WorldPackets::Auth::AuthContinuedSession>(std::move(packet));
            if (!authSession->ReadNoThrow())
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_CONTINUED_SESSION", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }
            HandleAuthContinuedSession(authSession);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        case CMSG_KEEP_ALIVE: /// @todo: handle this packet in the same way of CMSG_TIME_SYNC_RESPONSE
            sessionGuard.lock();
            LogOpcodeText(opcode, sessionGuard);
            if (_worldSession)
            {
                _worldSession->ResetTimeOutTime(true);
                return ReadDataHandlerResult::Ok;
            }
            LOG_ERROR("network", "WorldSocket::ReadDataHandler: client {} sent CMSG_KEEP_ALIVE without being authenticated", GetRemoteIpAddress().to_string());
            return ReadDataHandlerResult::Error;
        case CMSG_CONNECT_TO_FAILED:
        {
            sessionGuard.lock();
            LogOpcodeText(opcode, sessionGuard);
            try
            {
                HandleConnectToFailed(packet);
            }
            catch (ByteBufferException const&)
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_CONNECT_TO_FAILED", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }
            break;
        }
        case CMSG_ENTER_ENCRYPTED_MODE_ACK:
            LogOpcodeText(opcode, sessionGuard);
            HandleEnterEncryptedModeAck();
            break;
        case CMSG_LOG_DISCONNECT:
        {
            // The 54261 client sends this first, pre-auth, on disconnect. It is a no-op for us - just
            // consume the reason code so it is not treated as a "wrong opcode".
            LogOpcodeText(opcode, sessionGuard);
            uint32 reason = packet.read<uint32>();
            LOG_DEBUG("network", "WorldSocket::ReadDataHandler(): client {} sent CMSG_LOG_DISCONNECT (reason {})",
                GetRemoteIpAddress().to_string(), reason);
            break;
        }
        case CMSG_HOTFIX_REQUEST:
            _canRequestHotfixes = false;
            [[fallthrough]];
        default:
        {
            WorldPacket* packetToQueue;
            if (opcode == CMSG_TIME_SYNC_RESPONSE)
                packetToQueue = new WorldPacket(std::move(packet), GameTime::Now());
            else
                packetToQueue = new WorldPacket(std::move(packet));

            sessionGuard.lock();

            LogOpcodeText(opcode, sessionGuard);

            if (!_worldSession)
            {
                LOG_ERROR("network.opcode", "ProcessIncoming: Client not authed opcode = {}", uint32(opcode));
                delete packetToQueue;
                return ReadDataHandlerResult::Error;
            }

            OpcodeHandler const* handler = opcodeTable[opcode];
            if (!handler)
            {
                LOG_ERROR("network.opcode", "No defined handler for opcode {} sent by {}", GetOpcodeNameForLogging(static_cast<OpcodeClient>(packetToQueue->GetOpcode())), _worldSession->GetPlayerInfo());
                delete packetToQueue;
                return ReadDataHandlerResult::Error;
            }

            // Our Idle timer will reset on any non PING opcodes on login screen, allowing us to catch people idling.
            if (packetToQueue->GetOpcode() != CMSG_WARDEN3_DATA)
                _worldSession->ResetTimeOutTime(false);

            // Copy the packet to the heap before enqueuing
            _worldSession->QueuePacket(packetToQueue);
            break;
        }
    }

    return ReadDataHandlerResult::Ok;
}

void WorldSocket::LogOpcodeText(OpcodeClient opcode, std::unique_lock<std::mutex> const& guard) const
{
    if (!guard)
    {
        LOG_TRACE("network.opcode", "C->S: {} {}", GetRemoteIpAddress().to_string(), GetOpcodeNameForLogging(opcode));
    }
    else
    {
        LOG_TRACE("network.opcode", "C->S: {} {}", (_worldSession ? _worldSession->GetPlayerInfo() : GetRemoteIpAddress().to_string()),
            GetOpcodeNameForLogging(opcode));
    }
}

void WorldSocket::SendPacketAndLogOpcode(WorldPacket const& packet)
{
    LOG_TRACE("network.opcode", "S->C: {} {}", GetRemoteIpAddress().to_string(), GetOpcodeNameForLogging(static_cast<OpcodeServer>(packet.GetOpcode())));
    SendPacket(packet);
}

void WorldSocket::SendPacket(WorldPacket const& packet)
{
    if (!IsOpen())
        return;

    if (sPacketLog->CanLogPacket() && IsLoggingPackets())
        sPacketLog->LogPacket(packet, SERVER_TO_CLIENT, GetRemoteIpAddress(), GetRemotePort());

    _bufferQueue.Enqueue(new EncryptablePacket(packet, _authCrypt.IsInitialized()));
}

void WorldSocket::WritePacketToBuffer(EncryptablePacket const& packet, MessageBuffer& buffer)
{
    // 3.4.3.54261 wire opcodes are FLAT uint16 values
    uint16 opcode = uint16(packet.GetOpcode());
    uint32 packetSize = packet.size();

    // Reserve space for buffer
    uint8* headerPos = buffer.GetWritePointer();
    buffer.WriteCompleted(sizeof(PacketHeader));
    uint8* dataPos = buffer.GetWritePointer();
    buffer.WriteCompleted(sizeof(opcode));

    if (packetSize > MinSizeForCompression && packet.NeedsEncryption())
    {
        CompressedWorldPacket cmp;
        cmp.UncompressedSize = packetSize + sizeof(opcode);
        cmp.UncompressedAdler = adler32(adler32(0x9827D8F1, (Bytef*)&opcode, sizeof(opcode)), packet.contents(), packetSize);

        // Reserve space for compression info - uncompressed size and checksums
        uint8* compressionInfo = buffer.GetWritePointer();
        buffer.WriteCompleted(sizeof(CompressedWorldPacket));

        uint32 compressedSize = CompressPacket(buffer.GetWritePointer(), packet);

        cmp.CompressedAdler = adler32(0x9827D8F1, buffer.GetWritePointer(), compressedSize);

        memcpy(compressionInfo, &cmp, sizeof(CompressedWorldPacket));
        buffer.WriteCompleted(compressedSize);
        packetSize = compressedSize + sizeof(CompressedWorldPacket);

        opcode = uint16(SMSG_COMPRESSED_PACKET);
    }
    else if (!packet.empty())
        buffer.Write(packet.contents(), packet.size());

    memcpy(dataPos, &opcode, sizeof(opcode));
    packetSize += sizeof(opcode);

    // header{} zero-inits the GCM Tag so it matches the client's zero pre-encryption tag
    PacketHeader header{};
    header.Size = packetSize;
    _authCrypt.EncryptSend(dataPos, header.Size, header.Tag);

    memcpy(headerPos, &header, sizeof(PacketHeader));
}

uint32 WorldSocket::CompressPacket(uint8* buffer, WorldPacket const& packet)
{
    // 3.4.3.54261 wire opcodes are FLAT uint16 values
    uint16 opcode = uint16(packet.GetOpcode());
    uint32 bufferSize = deflateBound(_compressionStream, packet.size() + sizeof(opcode));

    _compressionStream->next_out = buffer;
    _compressionStream->avail_out = bufferSize;
    _compressionStream->next_in = (Bytef*)&opcode;
    _compressionStream->avail_in = sizeof(opcode);

    int32 z_res = deflate(_compressionStream, Z_NO_FLUSH);
    if (z_res != Z_OK)
    {
        LOG_ERROR("network", "Can't compress packet opcode (zlib: deflate) Error code: {} ({}, msg: {})", z_res, zError(z_res), _compressionStream->msg);
        return 0;
    }

    _compressionStream->next_in = (Bytef*)packet.contents();
    _compressionStream->avail_in = packet.size();

    z_res = deflate(_compressionStream, Z_SYNC_FLUSH);
    if (z_res != Z_OK)
    {
        LOG_ERROR("network", "Can't compress packet data (zlib: deflate) Error code: {} ({}, msg: {})", z_res, zError(z_res), _compressionStream->msg);
        return 0;
    }

    return bufferSize - _compressionStream->avail_out;
}

void WorldSocket::HandleAuthSession(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession)
{
    // 3.4.3 CMSG_AUTH_SESSION carries a JSON RealmJoinTicket identifying the game account + client variant.
    std::shared_ptr<JSON::RealmList::RealmJoinTicket> joinTicket = std::make_shared<JSON::RealmList::RealmJoinTicket>();
    if (!JSON::Deserialize(authSession->RealmJoinTicket, joinTicket.get()))
    {
        SendAuthResponseError(REALM_LIST_REALM_NOT_FOUND);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Failed to deserialize RealmJoinTicket from {}.", GetRemoteIpAddress().to_string());
        DelayedCloseSocket();
        return;
    }

    // Get the account information from the auth database
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_FOR_WORLD_AUTH);
    stmt->SetData(0, int32(realm.Id.Realm));
    stmt->SetData(1, joinTicket->gameaccount());

    QueueQuery(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback([this, authSession = std::move(authSession), joinTicket = std::move(joinTicket)](PreparedQueryResult result) mutable
    {
        HandleAuthSessionCallback(std::move(authSession), std::move(joinTicket), std::move(result));
    }));
}

void WorldSocket::HandleAuthSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession,
    std::shared_ptr<JSON::RealmList::RealmJoinTicket> joinTicket, PreparedQueryResult result)
{
    // Stop if the account is not found
    if (!result)
    {
        // We can not log here, as we do not know the account. Thus, no accountId.
        SendAuthResponseError(AUTH_UNKNOWN_ACCOUNT);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Sent Auth Response (unknown account).");
        DelayedCloseSocket();
        return;
    }

    AccountInfo account(result->Fetch());

    // For hook purposes, we get Remoteaddress at this point.
    std::string address = sConfigMgr->GetOption<bool>("AllowLoggingIPAddressesInDatabase", true, true) ? GetRemoteIpAddress().to_string() : "0.0.0.0";

    // ---------------------------------------------------------------------------------------------
    // 3.4.3.54261 world-auth crypto. The digest is HMAC_SHA256 over LocalChallenge || _serverChallenge ||
    // AuthCheckSeed, keyed by SHA256(KeyData || perBuildAuthSeed). For build 54261 the 64-bit Windows
    // client uses win64AuthSeed (the only seed Blizzard set for this build; mac64/win are null). The
    // 16-byte value comes from the 54261 `build_info` row, cross-verified across community 3.4.3 forks.
    // (We only support the patched Win64 client, so the seed is appended unconditionally.)
    static constexpr std::array<uint8, 16> Win64AuthSeed =
        { 0x25, 0xFD, 0x81, 0x24, 0x75, 0xDC, 0xF2, 0x6F, 0x9F, 0x13, 0x83, 0xAE, 0xD3, 0x7F, 0xC9, 0x9E };

    ClientBuild::VariantId buildVariant = { joinTicket->platform(), joinTicket->clientarch(), joinTicket->type() };

    // digestKeyHash = SHA256(KeyData || win64AuthSeed)
    Acore::Crypto::SHA256 digestKeyHash;
    digestKeyHash.UpdateData(account.KeyData.data(), account.KeyData.size());
    digestKeyHash.UpdateData(Win64AuthSeed.data(), Win64AuthSeed.size());
    digestKeyHash.Finalize();

    // serverDigest = HMAC_SHA256(digestKeyHash)( LocalChallenge || _serverChallenge || AuthCheckSeed )
    Acore::Crypto::HMAC_SHA256 hmac(digestKeyHash.GetDigest());
    hmac.UpdateData(authSession->LocalChallenge);
    hmac.UpdateData(_serverChallenge);
    hmac.UpdateData(AuthCheckSeed);
    hmac.Finalize();

    // Check that Key and account name are the same on client and server
    if (memcmp(hmac.GetDigest().data(), authSession->Digest.data(), authSession->Digest.size()) != 0)
    {
        SendAuthResponseError(AUTH_FAILED);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Authentication failed for account: {} ('{}') address: {}. "
            "Digest mismatch (keyData[0]: {:02X}, serverDigest[0]: {:02X}, clientDigest[0]: {:02X}).",
            account.Id, joinTicket->gameaccount(), address, account.KeyData[0], hmac.GetDigest()[0], authSession->Digest[0]);
        DelayedCloseSocket();
        return;
    }

    // sessionKey = SessionKeyGenerator( HMAC_SHA256( SHA256(KeyData) )( _serverChallenge || LocalChallenge || SessionKeySeed ) )
    Acore::Crypto::SHA256 keyData;
    keyData.UpdateData(account.KeyData.data(), account.KeyData.size());
    keyData.Finalize();

    Acore::Crypto::HMAC_SHA256 sessionKeyHmac(keyData.GetDigest());
    sessionKeyHmac.UpdateData(_serverChallenge);
    sessionKeyHmac.UpdateData(authSession->LocalChallenge);
    sessionKeyHmac.UpdateData(SessionKeySeed);
    sessionKeyHmac.Finalize();

    SessionKeyGenerator<Acore::Crypto::SHA256> sessionKeyGenerator(sessionKeyHmac.GetDigest());
    sessionKeyGenerator.Generate(_sessionKey.data(), 40);

    // _encryptKey = first 16 bytes of HMAC_SHA256(_sessionKey)( LocalChallenge || _serverChallenge || EncryptionKeySeed )
    Acore::Crypto::HMAC_SHA256 encryptKeyGen(_sessionKey);
    encryptKeyGen.UpdateData(authSession->LocalChallenge);
    encryptKeyGen.UpdateData(_serverChallenge);
    encryptKeyGen.UpdateData(EncryptionKeySeed);
    encryptKeyGen.Finalize();

    // only first 16 bytes of the hmac are used (AES-128 key)
    memcpy(_encryptKey.data(), encryptKeyGen.GetDigest().data(), 16);

    LoginDatabasePreparedStatement* stmt = nullptr;

    // As we don't know if attempted login process by ip works, we update last_attempt_ip right away
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_ATTEMPT_IP);
    stmt->SetData(0, address);
    stmt->SetData(1, joinTicket->gameaccount());
    LoginDatabase.Execute(stmt);
    // This also allows to check for possible "hack" attempts on account

    // Persist the derived 40-byte world session key (overwrites the 64-byte bnet blob) so a follow-up
    // continued (instance) session can reload it.
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_INFO_CONTINUED_SESSION);
    stmt->SetData(0, _sessionKey);
    stmt->SetData(1, account.Id);
    LoginDatabase.Execute(stmt);

    // First reject the connection if packet contains invalid data or realm state doesn't allow logging in
    if (sWorld->IsClosed())
    {
        SendAuthResponseError(AUTH_REJECT);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: World closed, denying client ({}).", GetRemoteIpAddress().to_string());
        DelayedCloseSocket();
        return;
    }

    if (authSession->RealmID != realm.Id.Realm)
    {
        SendAuthResponseError(REALM_LIST_REALM_NOT_FOUND);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} requested connecting with realm id {} but this realm has id {} set in config.",
            GetRemoteIpAddress().to_string(), authSession->RealmID, realm.Id.Realm);
        DelayedCloseSocket();
        return;
    }

    // Must be done before WorldSession is created
    bool wardenActive = sWorld->getBoolConfig(CONFIG_WARDEN_ENABLED);
    // 3.4.3 client OS strings are the modern 4-char forms (Wn64 = Win64, Mc64 = macOS64); the legacy
    // 3.3.5a "Win"/"OSX" are replaced. Warden only supports these (no 3.4.3 Warden module yet anyway).
    if (wardenActive && account.OS != "Win" && account.OS != "Wn64" && account.OS != "Mc64")
    {
        SendAuthResponseError(AUTH_REJECT);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} attempted to log in using invalid client OS ({}).", address, account.OS);
        DelayedCloseSocket();
        return;
    }

    if (IpLocationRecord const* location = sIPLocation->GetLocationRecord(address))
        _ipCountry = location->CountryCode;

    ///- Re-check ip locking (same check as in auth).
    if (account.IsLockedToIP)
    {
        if (account.LastIP != address)
        {
            SendAuthResponseError(AUTH_FAILED);
            LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account IP differs. Original IP: {}, new IP: {}).", account.LastIP, address);
            // We could log on hook only instead of an additional db log, however action logger is config based. Better keep DB logging as well
            sScriptMgr->OnFailedAccountLogin(account.Id);
            DelayedCloseSocket();
            return;
        }
    }
    else if (!account.LockCountry.empty() && account.LockCountry != "00" && !_ipCountry.empty())
    {
        if (account.LockCountry != _ipCountry)
        {
            SendAuthResponseError(AUTH_FAILED);
            LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account country differs. Original country: {}, new country: {}).", account.LockCountry, _ipCountry);
            // We could log on hook only instead of an additional db log, however action logger is config based. Better keep DB logging as well
            sScriptMgr->OnFailedAccountLogin(account.Id);
            DelayedCloseSocket();
            return;
        }
    }

    //! Negative mutetime indicates amount of minutes to be muted effective on next login - which is now.
    if (account.MuteTime < 0)
    {
        account.MuteTime = GameTime::GetGameTime().count() + std::llabs(account.MuteTime);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME_LOGIN);
        stmt->SetData(0, account.MuteTime);
        stmt->SetData(1, account.Id);
        LoginDatabase.Execute(stmt);
    }

    if (account.IsBanned)
    {
        SendAuthResponseError(AUTH_BANNED);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account banned).");
        sScriptMgr->OnFailedAccountLogin(account.Id);
        DelayedCloseSocket();
        return;
    }

    // Check locked state for server
    AccountTypes allowedAccountType = sWorld->GetPlayerSecurityLimit();
    LOG_DEBUG("network", "Allowed Level: {} Player Level {}", allowedAccountType, account.Security);
    if (allowedAccountType > SEC_PLAYER && account.Security < allowedAccountType)
    {
        SendAuthResponseError(AUTH_UNAVAILABLE);
        LOG_DEBUG("network", "WorldSocket::HandleAuthSession: User tries to login but his security level is not enough");
        sScriptMgr->OnFailedAccountLogin(account.Id);
        DelayedCloseSocket();
        return;
    }

    LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Client '{}' authenticated successfully from {}.", joinTicket->gameaccount(), address);

    // Update the last_ip in the database as it was successful for login
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_IP);
    stmt->SetData(0, address);
    stmt->SetData(1, joinTicket->gameaccount());

    LoginDatabase.Execute(stmt);

    // At this point, we can safely hook a successful login
    sScriptMgr->OnAccountLogin(account.Id);

    _authed = true;

    sScriptMgr->OnLastIpUpdate(account.Id, address);

    // TODO(3.4.3 brick-E2b): battlenetAccountId is not provided by LOGIN_SEL_ACCOUNT_INFO_FOR_WORLD_AUTH;
    // passing 0 for now. TotalTime is likewise not selected here (was account.totaltime in the legacy
    // SELECT) - pass 0 until the world-auth SELECT carries it.
    _worldSession = new WorldSession(account.Id, std::move(*joinTicket->mutable_gameaccount()), account.Flags, 0 /*battlenetAccountId*/,
        std::static_pointer_cast<WorldSocket>(shared_from_this()), account.Security, account.Expansion, account.MuteTime,
        account.OS, account.TimezoneOffset, account.Build, buildVariant, account.Locale,
        account.Recruiter, account.IsRectuiter, account.Security ? true : false, 0 /*TotalTime*/);

    // TODO(3.4.3 brick-E2b): the modern CMSG_AUTH_SESSION does not carry the legacy addon blob; addon
    // info now arrives via a separate path. ReadAddonsInfo is intentionally not called here.

    // Initialize Warden system only if it is enabled by config
    if (wardenActive)
        _worldSession->InitWarden(_sessionKey, account.OS);

    _worldSession->ValidateAccountFlags();

    QueueQuery(_worldSession->LoadPermissionsAsync().WithPreparedCallback(std::bind(&WorldSocket::LoadSessionPermissionsCallback, this, std::placeholders::_1)));
    AsyncRead(Acore::Net::InvokeReadHandlerCallback<WorldSocket>{ .Socket = this });
}

void WorldSocket::LoadSessionPermissionsCallback(PreparedQueryResult result)
{
    // RBAC must be loaded before adding session to check for skip queue permission
    _worldSession->GetRBACData()->LoadFromDBCallback(result);

    // 3.4.3: arm AES-128-GCM only after the client acks. Send SMSG_ENTER_ENCRYPTED_MODE here and defer
    // AddSession to HandleEnterEncryptedModeAck.
    SendPacketAndLogOpcode(*WorldPackets::Auth::EnterEncryptedMode(_encryptKey, true).Write());

    // The auth-session read returned WaitingForQuery (read loop paused); resume reading now so we
    // receive the client's CMSG_ENTER_ENCRYPTED_MODE_ACK that completes the handshake.
    AsyncRead(Acore::Net::InvokeReadHandlerCallback<WorldSocket>{ .Socket = this });
}

void WorldSocket::HandleAuthContinuedSession(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession)
{
    WorldSession::ConnectToKey key;
    key.Raw = authSession->Key;

    _type = ConnectionType(key.Fields.ConnectionType);
    if (_type != CONNECTION_TYPE_INSTANCE)
    {
        SendAuthResponseError(AUTH_FAILED);
        DelayedCloseSocket();
        return;
    }

    uint32 accountId = uint32(key.Fields.AccountId);
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_CONTINUED_SESSION);
    stmt->SetData(0, accountId);

    QueueQuery(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback([this, authSession = std::move(authSession)](PreparedQueryResult result) mutable
    {
        HandleAuthContinuedSessionCallback(std::move(authSession), std::move(result));
    }));
}

void WorldSocket::HandleAuthContinuedSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession, PreparedQueryResult result)
{
    if (!result)
    {
        SendAuthResponseError(AUTH_FAILED);
        DelayedCloseSocket();
        return;
    }

    WorldSession::ConnectToKey key;
    _key = key.Raw = authSession->Key;

    uint32 accountId = uint32(key.Fields.AccountId);
    Field* fields = result->Fetch();
    std::string login = fields[0].Get<std::string>();
    _sessionKey = fields[1].Get<Binary, SESSION_KEY_LENGTH>();

    // TODO(3.4.3 brick-E2b): verify HMAC_SHA512(_sessionKey)(Key||LocalChallenge||_serverChallenge||
    // ContinuedSessionSeed) against authSession->Digest, derive _encryptKey, send SMSG_ENTER_ENCRYPTED_MODE
    // and register the instance socket with its WorldSession (needs WorldSessionMgr::AddInstanceSocket /
    // WorldSession::AddInstanceConnection, both deferred to E2b). For now reject the 2nd socket so the
    // realm path (char-select) is unaffected.
    LOG_DEBUG("network", "WorldSocket::HandleAuthContinuedSession: stubbed continued-session for account {} ('{}') - awaiting brick E2b", accountId, login);

    SendAuthResponseError(AUTH_FAILED);
    DelayedCloseSocket();
}

void WorldSocket::HandleConnectToFailed(WorldPacket& recvPacket)
{
    // TODO(3.4.3 brick-E2): drive WorldSession::SendConnectToInstance retries / AbortLogin based on
    // the ConnectToSerial in the packet. Parsed here so the payload is consumed; behaviour is E2.
    uint8 serial;
    recvPacket >> serial;
    (void)serial;
}

void WorldSocket::HandleEnterEncryptedModeAck()
{
    // Arm AES-128-GCM with the derived key, then complete the auth flow.
    _authCrypt.Init(_encryptKey);

    if (_type == CONNECTION_TYPE_REALM)
    {
        sWorldSessionMgr->AddSession(_worldSession);
    }
    else
    {
        // TODO(3.4.3 brick-E2b): register the instance socket onto its WorldSession
        // (sWorldSessionMgr has no AddInstanceSocket yet). The continued-session path currently
        // rejects before reaching here, so this branch is unreachable for now.
    }
}

void WorldSocket::SendAuthResponseError(uint32 code)
{
    // 3.4.3 SMSG_AUTH_RESPONSE: a uint32 Result followed by bit-packed success/wait blocks. Use the
    // brick-C packet so the error response is wire-correct (the legacy 1-byte form did not parse).
    // NOTE: `code` is still an AzerothCore ResponseCodes value (not a Battlenet RpcErrorCode like TC
    // uses); the client treats any non-success Result as a denial, so error reporting still works.
    // TODO(3.4.3 brick-E2b): map these to proper Battlenet RpcErrorCodes for accurate client messages.
    WorldPackets::Auth::AuthResponse response;
    response.Result = code;
    SendPacketAndLogOpcode(*response.Write());
}

bool WorldSocket::HandlePing(WorldPacket& recvPacket)
{
    using namespace std::chrono;

    uint32 ping;
    uint32 latency;

    // Get the ping packet content
    recvPacket >> ping;
    recvPacket >> latency;

    if (_LastPingTime == steady_clock::time_point())
    {
        _LastPingTime = steady_clock::now();
    }
    else
    {
        steady_clock::time_point now = steady_clock::now();
        steady_clock::duration diff = now - _LastPingTime;

        _LastPingTime = now;

        if (diff < seconds(27))
        {
            ++_OverSpeedPings;

            uint32 maxAllowed = sWorld->getIntConfig(CONFIG_MAX_OVERSPEED_PINGS);

            if (maxAllowed && _OverSpeedPings > maxAllowed)
            {
                std::unique_lock<std::mutex> sessionGuard(_worldSessionLock);

                if (_worldSession && !_worldSession->HasPermission(rbac::RBAC_PERM_SKIP_CHECK_OVERSPEED_PING))
                {
                    LOG_ERROR("network", "WorldSocket::HandlePing: {} kicked for over-speed pings (address: {})",
                        _worldSession->GetPlayerInfo(), GetRemoteIpAddress().to_string());

                    return false;
                }
            }
        }
        else
        {
            _OverSpeedPings = 0;
        }
    }

    {
        std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);

        if (_worldSession)
            _worldSession->SetLatency(latency);
        else
        {
            LOG_ERROR("network", "WorldSocket::HandlePing: peer sent CMSG_PING, but is not authenticated or got recently kicked, address = {}", GetRemoteIpAddress().to_string());
            return false;
        }
    }

    WorldPacket packet(SMSG_PONG, 4);
    packet << ping;
    SendPacketAndLogOpcode(packet);

    return true;
}
