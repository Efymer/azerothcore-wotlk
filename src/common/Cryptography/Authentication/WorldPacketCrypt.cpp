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

#include "WorldPacketCrypt.h"
#include <array>
#include <cstring>
#include <vector>

WorldPacketCrypt::WorldPacketCrypt() : _clientDecrypt(false, 128), _serverEncrypt(true, 128), _clientCounter(0), _serverCounter(0), _initialized(false)
{
}

void WorldPacketCrypt::Init(Key const& key)
{
    _clientDecrypt.Init(key);
    _serverEncrypt.Init(key);
    _initialized = true;
}

struct WorldPacketCryptIV
{
    WorldPacketCryptIV(uint64 counter, uint32 magic)
    {
        memcpy(Value.data(), &counter, sizeof(uint64));
        memcpy(Value.data() + sizeof(uint64), &magic, sizeof(uint32));
    }

    std::array<uint8, 12> Value;
};

bool WorldPacketCrypt::PeekDecryptRecv(uint8* data, std::size_t length)
{
    if (_initialized)
    {
        WorldPacketCryptIV iv{ _clientCounter, 0x544E4C43 };
        if (!_clientDecrypt.ProcessNoIntegrityCheck(iv.Value, data, length))
            return false;
    }

    return true;
}

bool WorldPacketCrypt::DecryptRecv(uint8* data, std::size_t length, Acore::Crypto::AES::Tag& tag)
{
    if (!_initialized)
    {
        memset(tag, 0, sizeof(tag));
        ++_clientCounter;
        return true;
    }

    // The 3.4.3.54261 client can advance its outgoing GCM nonce counter without transmitting a packet:
    // it encrypts a queued request (e.g. a Battle.net store query), then drops the send after a server
    // reply, leaving a forward gap in the client->server counter sequence. Strict sequential decryption
    // would then mismatch every following packet and tear down the session. Probe a small forward window
    // so a skipped counter resyncs us. The GCM tag still authenticates each packet, and the counter only
    // ever moves forward, so this opens no replay/forgery window. `data` is decrypted in place, so retries
    // restore the ciphertext from a saved copy and verify against an untouched copy of the tag.
    std::vector<uint8> cipher(data, data + length);
    for (uint64 candidate = _clientCounter; candidate <= _clientCounter + MaxRecvCounterSkip; ++candidate)
    {
        memcpy(data, cipher.data(), length);
        Acore::Crypto::AES::Tag tagCopy;
        memcpy(tagCopy, tag, sizeof(tagCopy));

        WorldPacketCryptIV iv{ candidate, 0x544E4C43 };
        if (_clientDecrypt.Process(iv.Value, data, length, tagCopy))
        {
            _clientCounter = candidate + 1;
            return true;
        }
    }

    return false;
}

bool WorldPacketCrypt::EncryptSend(uint8* data, std::size_t length, Acore::Crypto::AES::Tag& tag)
{
    if (_initialized)
    {
        WorldPacketCryptIV iv{ _serverCounter, 0x52565253 };
        if (!_serverEncrypt.Process(iv.Value, data, length, tag))
            return false;
    }
    else
        memset(tag, 0, sizeof(tag));

    ++_serverCounter;
    return true;
}
