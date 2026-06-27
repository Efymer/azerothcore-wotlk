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
#include "Config.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "IPLocation.h"
#include "IpBanCheckConnectionInitializer.h"
#include "Opcodes.h"
#include "PacketLog.h"
#include "Random.h"
#include "Realm.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include "RBAC.h"
#include "zlib.h"
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
        uint32 packetSize = queued->size() + 4 /*opcode*/;
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
    Acore::Crypto::GetRandomBytes(_serverChallenge);

    // SMSG_AUTH_CHALLENGE (3.4.3): DosChallenge[32] + Challenge[32] + DosZeroBits(uint8)
    WorldPacket packet(SMSG_AUTH_CHALLENGE, 32 + 32 + 1);
    packet.append(Acore::Crypto::GetRandomBytes<32>());     // DosChallenge
    packet.append(_serverChallenge);                        // server challenge
    packet << uint8(1);                                     // DosZeroBits

    SendPacketAndLogOpcode(packet);
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
    uint32 encryptedOpcode = header->EncryptedOpcode;

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

struct ClientAuthSession
{
    uint32 BattlegroupID = 0;
    uint32 LoginServerType = 0;
    uint32 RealmID = 0;
    uint32 Build = 0;
    std::array<uint8, 4> LocalChallenge = {};
    uint32 LoginServerID = 0;
    uint32 RegionID = 0;
    uint64 DosResponse = 0;
    Acore::Crypto::SHA1::Digest Digest = {};
    std::string Account;
    ByteBuffer AddonInfo;
};

struct ClientAuthContinuedSession
{
    uint64 Key = 0;
    std::array<uint8, 16> LocalChallenge = {};
    std::array<uint8, 24> Digest = {};
};

struct AccountInfo
{
    uint32 Id;
    ::SessionKey SessionKey;
    std::string LastIP;
    bool IsLockedToIP;
    std::string LockCountry;
    uint8 Expansion;
    uint32 Flags;
    int64 MuteTime;
    LocaleConstant Locale;
    uint32 Recruiter;
    std::string OS;
    bool IsRectuiter;
    AccountTypes Security;
    bool IsBanned;
    uint32 TotalTime;

    explicit AccountInfo(Field* fields)
    {
        //           0             1          2         3               4            5        6          7         8            9    10           11          12
        // SELECT a.id, a.sessionkey, a.last_ip, a.locked, a.lock_country, a.expansion, a.Flags a.mutetime, a.locale, a.recruiter, a.os, a.totaltime, aa.gmLevel,
        //                                                           13    14
        // ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, r.id
        // FROM account a
        // LEFT JOIN account_access aa ON a.id = aa.AccountID AND aa.RealmID IN (-1, ?)
        // LEFT JOIN account_banned ab ON a.id = ab.id
        // LEFT JOIN account r ON a.id = r.recruiter
        // WHERE a.username = ? ORDER BY aa.RealmID DESC LIMIT 1
        Id = fields[0].Get<uint32>();
        SessionKey = fields[1].Get<Binary, SESSION_KEY_LENGTH>();
        LastIP = fields[2].Get<std::string>();
        IsLockedToIP = fields[3].Get<bool>();
        LockCountry = fields[4].Get<std::string>();
        Expansion = fields[5].Get<uint8>();
        Flags = fields[6].Get<uint32>();
        MuteTime = fields[7].Get<int64>();
        Locale = LocaleConstant(fields[8].Get<uint8>());
        Recruiter = fields[9].Get<uint32>();
        OS = fields[10].Get<std::string>();
        TotalTime = fields[11].Get<uint32>();
        Security = AccountTypes(fields[12].Get<uint8>());
        IsBanned = fields[13].Get<uint64>() != 0;
        IsRectuiter = fields[14].Get<uint32>() != 0;

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
    OpcodeClient opcode = packet.read<OpcodeClient>();
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

            try
            {
                HandleAuthSession(packet);
                return ReadDataHandlerResult::WaitingForQuery;
            }
            catch (ByteBufferException const&) { }

            LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_SESSION", GetRemoteIpAddress().to_string());
            return ReadDataHandlerResult::Error;
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

            try
            {
                HandleAuthContinuedSession(packet);
                return ReadDataHandlerResult::WaitingForQuery;
            }
            catch (ByteBufferException const&) { }

            LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_CONTINUED_SESSION", GetRemoteIpAddress().to_string());
            return ReadDataHandlerResult::Error;
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
    uint32 opcode = packet.GetOpcode();
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

        opcode = SMSG_COMPRESSED_PACKET;
    }
    else if (!packet.empty())
        buffer.Write(packet.contents(), packet.size());

    memcpy(dataPos, &opcode, sizeof(opcode));
    packetSize += sizeof(opcode);

    PacketHeader header;
    header.Size = packetSize;
    _authCrypt.EncryptSend(dataPos, header.Size, header.Tag);

    memcpy(headerPos, &header, sizeof(PacketHeader));
}

uint32 WorldSocket::CompressPacket(uint8* buffer, WorldPacket const& packet)
{
    uint32 opcode = packet.GetOpcode();
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

void WorldSocket::HandleAuthSession(WorldPacket& recvPacket)
{
    std::shared_ptr<ClientAuthSession> authSession = std::make_shared<ClientAuthSession>();

    // TODO(3.4.3 brick-E2): the real 3.4.3 CMSG_AUTH_SESSION carries a JSON RealmJoinTicket plus
    // the bnet RegionRealmBattlegroup/LocalChallenge/Digest in the modern bit-packed layout. For
    // brick E1 we best-effort parse the legacy SRP field order so the WorldSession can be built;
    // the digest/key-derivation is left for E2.
    recvPacket >> authSession->Build;
    recvPacket >> authSession->LoginServerID;
    recvPacket >> authSession->Account;
    recvPacket >> authSession->LoginServerType;
    recvPacket.read(authSession->LocalChallenge);
    recvPacket >> authSession->RegionID;
    recvPacket >> authSession->BattlegroupID;
    recvPacket >> authSession->RealmID;               // realmId from auth_database.realmlist table
    recvPacket >> authSession->DosResponse;
    recvPacket.read(authSession->Digest);
    authSession->AddonInfo.resize(recvPacket.size() - recvPacket.rpos());
    recvPacket.read(authSession->AddonInfo.contents(), authSession->AddonInfo.size()); // .contents will throw if empty, thats what we want

    // Get the account information from the auth database
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME);
    stmt->SetData(0, int32(realm.Id.Realm));
    stmt->SetData(1, authSession->Account);

    QueueQuery(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSocket::HandleAuthSessionCallback, this, authSession, std::placeholders::_1)));
}

void WorldSocket::HandleAuthSessionCallback(std::shared_ptr<ClientAuthSession> authSession, PreparedQueryResult result)
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

    LoginDatabasePreparedStatement* stmt = nullptr;

    // As we don't know if attempted login process by ip works, we update last_attempt_ip right away
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_ATTEMPT_IP);
    stmt->SetData(0, address);
    stmt->SetData(1, authSession->Account);
    LoginDatabase.Execute(stmt);
    // This also allows to check for possible "hack" attempts on account

    // TODO(3.4.3 brick-E2): derive _sessionKey/_encryptKey from the modern handshake
    // (SHA512/HMAC over the four 3.4.3 seeds: AuthCheckSeed, SessionKeySeed, ContinuedSessionSeed,
    // EncryptionKeySeed) and verify authSession->Digest. For brick E1 we keep the account session
    // key so the WorldSession can be built, but skip the digest check and leave the AES-GCM crypt
    // uninitialized (HandleEnterEncryptedModeAck would Init it once E2 fills _encryptKey).
    _sessionKey = account.SessionKey;

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
    if (wardenActive && account.OS != "Win" && account.OS != "OSX")
    {
        SendAuthResponseError(AUTH_REJECT);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} attempted to log in using invalid client OS ({}).", address, account.OS);
        DelayedCloseSocket();
        return;
    }

    // TODO(3.4.3 brick-E2): digest verification of authSession->Digest against the HMAC of the
    // local challenge + server challenge + AuthCheckSeed. Skipped in brick E1.

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

    LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Client '{}' authenticated successfully from {}.", authSession->Account, address);

    // Update the last_ip in the database as it was successful for login
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_IP);
    stmt->SetData(0, address);
    stmt->SetData(1, authSession->Account);

    LoginDatabase.Execute(stmt);

    // At this point, we can safely hook a successful login
    sScriptMgr->OnAccountLogin(account.Id);

    _authed = true;

    sScriptMgr->OnLastIpUpdate(account.Id, address);

    // TODO(3.4.3 brick-E2): real values from the modern handshake (battlenet account id, timezone
    // offset, client build variant). For now the legacy SRP handshake supplies what it can and the
    // rest are sensible placeholders so the modern WorldSession ctor compiles.
    _worldSession = new WorldSession(account.Id, std::move(authSession->Account), account.Flags, 0 /*battlenetAccountId*/,
        std::static_pointer_cast<WorldSocket>(shared_from_this()), account.Security, account.Expansion, account.MuteTime,
        account.OS, Minutes(0) /*timezoneOffset*/, authSession->Build, ClientBuild::VariantId{} /*clientBuildVariant*/,
        account.Locale, account.Recruiter, account.IsRectuiter, account.Security ? true : false, account.TotalTime);

    _worldSession->ReadAddonsInfo(authSession->AddonInfo);

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

    // TODO(3.4.3 brick-E2): once _encryptKey is derived, send SMSG_ENTER_ENCRYPTED_MODE here and
    // defer AddSession to HandleEnterEncryptedModeAck (modern AES-GCM goes live only after the
    // client acks). For brick E1 we add the session immediately (crypt stays uninitialized).
    sWorldSessionMgr->AddSession(_worldSession);
}

void WorldSocket::HandleAuthContinuedSession(WorldPacket& recvPacket)
{
    std::shared_ptr<ClientAuthContinuedSession> authSession = std::make_shared<ClientAuthContinuedSession>();

    recvPacket >> authSession->Key;
    recvPacket.read(authSession->LocalChallenge);
    recvPacket.read(authSession->Digest);

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

    QueueQuery(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSocket::HandleAuthContinuedSessionCallback, this, authSession, std::placeholders::_1)));
}

void WorldSocket::HandleAuthContinuedSessionCallback(std::shared_ptr<ClientAuthContinuedSession> authSession, PreparedQueryResult result)
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

    // TODO(3.4.3 brick-E2): verify the HMAC(_sessionKey, Key||LocalChallenge||ServerChallenge||
    // ContinuedSessionSeed) digest, derive _encryptKey, then drive SMSG_ENTER_ENCRYPTED_MODE and
    // wire the freshly authenticated instance socket onto its WorldSession via
    // WorldSession::AddInstanceConnection (sWorldSessionMgr currently has no AddInstanceSocket).
    LOG_DEBUG("network", "WorldSocket::HandleAuthContinuedSession: stubbed continued-session for account {} ('{}') - awaiting brick E2", accountId, login);

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
    // TODO(3.4.3 brick-E2): _authCrypt.Init(_encryptKey) and add the session / instance socket here
    // once _encryptKey is derived in the auth callbacks. No-op in brick E1 (crypt stays disabled).
}

void WorldSocket::SendAuthResponseError(uint8 code)
{
    WorldPacket packet(SMSG_AUTH_RESPONSE, 1);
    packet << uint8(code);

    SendPacketAndLogOpcode(packet);
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
