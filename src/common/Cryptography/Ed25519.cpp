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

#include "Ed25519.h"
#include "Memory.h"
#include <openssl/core_names.h>
#include <openssl/opensslv.h>
#include <openssl/params.h>
#include <openssl/pem.h>
#include <memory>

// SignWithContext (Ed25519ctx) selects RFC 8032 ctx mode via the OSSL_SIGNATURE_PARAM_INSTANCE /
// CONTEXT_STRING params, which OpenSSL only honours from 3.2 onward. On older OpenSSL those params are
// silently ignored, producing a plain-Ed25519 signature the WoW client rejects — fail the build instead.
static_assert(OPENSSL_VERSION_NUMBER >= 0x30200000L,
    "Ed25519::SignWithContext requires OpenSSL >= 3.2 for the instance/context-string signature params");

namespace Acore::Crypto
{
    Ed25519::Ed25519() = default;

    Ed25519::Ed25519(Ed25519 const& right)
    {
        *this = right;
    }

    Ed25519::Ed25519(Ed25519&& right) noexcept
    {
        *this = std::move(right);
    }

    Ed25519::~Ed25519()
    {
        EVP_PKEY_free(_key);
    }

    Ed25519& Ed25519::operator=(Ed25519 const& right)
    {
        if (this == &right)
            return *this;

        _key = right._key;                      // EVP_PKEY uses reference counting internally, just copy the pointer
        EVP_PKEY_up_ref(_key);                  // Bump reference count for PKEY, as every instance of this class holds two references to PKEY and destructor decrements it twice
        return *this;
    }

    Ed25519& Ed25519::operator=(Ed25519&& right) noexcept
    {
        if (this == &right)
            return *this;

        _key = std::exchange(right._key, EVP_PKEY_new());
        return *this;
    }

    bool Ed25519::LoadFromFile(std::string const& fileName)
    {
        if (_key)
        {
            EVP_PKEY_free(_key);
            _key = nullptr;
        }

        auto keyBIO = make_unique_ptr_with_deleter<&BIO_free>(BIO_new_file(fileName.c_str(), "r"));
        if (!keyBIO)
            return false;

        _key = EVP_PKEY_new();
        if (!PEM_read_bio_PrivateKey(keyBIO.get(), &_key, nullptr, nullptr))
            return false;

        return true;
    }

    bool Ed25519::LoadFromString(std::string const& keyPem)
    {
        if (_key)
        {
            EVP_PKEY_free(_key);
            _key = nullptr;
        }

        auto keyBIO = make_unique_ptr_with_deleter<&BIO_free>(BIO_new_mem_buf(
            const_cast<char*>(keyPem.c_str()) /*api hack - this function assumes memory is readonly but lacks const modifier*/,
            keyPem.length() + 1));
        if (!keyBIO)
            return false;

        _key = EVP_PKEY_new();
        if (!PEM_read_bio_PrivateKey(keyBIO.get(), &_key, nullptr, nullptr))
            return false;

        return true;
    }

    bool Ed25519::LoadFromByteArray(std::array<uint8, 32> const& keyBytes)
    {
        if (_key)
        {
            EVP_PKEY_free(_key);
            _key = nullptr;
        }

        _key = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, keyBytes.data(), keyBytes.size());
        if (!_key)
            return false;

        return true;
    }

    bool Ed25519::Sign(uint8 const* message, std::size_t messageLength, std::vector<uint8>& output)
    {
        auto ctx = make_unique_ptr_with_deleter<&EVP_MD_CTX_free>(EVP_MD_CTX_new());
        if (!ctx)
            return false;

        // Ed25519 is a one-shot signature scheme: no digest is supplied and EVP_DigestSign must be used (not Update/Final)
        if (EVP_DigestSignInit_ex(ctx.get(), nullptr, nullptr, nullptr, nullptr, _key, nullptr) == 0)
            return false;

        std::size_t signatureLength = 0;
        if (EVP_DigestSign(ctx.get(), nullptr, &signatureLength, message, messageLength) == 0)
            return false;

        output.resize(signatureLength);
        return EVP_DigestSign(ctx.get(), output.data(), &signatureLength, message, messageLength) != 0;
    }

    bool Ed25519::SignWithContext(uint8 const* message, std::size_t messageLength, std::vector<uint8> const& context, std::vector<uint8>& output)
    {
        auto ctx = make_unique_ptr_with_deleter<&EVP_MD_CTX_free>(EVP_MD_CTX_new());
        if (!ctx)
            return false;

        // Ed25519ctx (RFC 8032): a non-empty context string is mixed into the signature.
        // OpenSSL 3.2+ exposes this through the "instance" and "context-string" signature parameters.
        char instance[] = "Ed25519ctx";
        OSSL_PARAM params[] =
        {
            OSSL_PARAM_construct_utf8_string(OSSL_SIGNATURE_PARAM_INSTANCE, instance, 0),
            OSSL_PARAM_construct_octet_string(OSSL_SIGNATURE_PARAM_CONTEXT_STRING, const_cast<uint8*>(context.data()), context.size()),
            OSSL_PARAM_construct_end()
        };

        if (EVP_DigestSignInit_ex(ctx.get(), nullptr, nullptr, nullptr, nullptr, _key, params) == 0)
            return false;

        std::size_t signatureLength = 0;
        if (EVP_DigestSign(ctx.get(), nullptr, &signatureLength, message, messageLength) == 0)
            return false;

        output.resize(signatureLength);
        return EVP_DigestSign(ctx.get(), output.data(), &signatureLength, message, messageLength) != 0;
    }
}
