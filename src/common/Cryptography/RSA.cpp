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

#include "RSA.h"
#include "CryptoHash.h"
#include "Memory.h"
#include <openssl/core_names.h>
#include <openssl/params.h>
#include <openssl/pem.h>
#include <algorithm>
#include <memory>
#include <vector>

namespace Acore::Crypto
{
    void RsaSignature::DigestGenerator::EVP_MD_Deleter::operator()(EVP_MD* md) const
    {
        EVP_MD_free(md);
    }

    std::unique_ptr<EVP_MD, RsaSignature::DigestGenerator::EVP_MD_Deleter> RsaSignature::SHA256::GetGenerator() const
    {
        return std::unique_ptr<EVP_MD, EVP_MD_Deleter>(EVP_MD_fetch(nullptr, OSSL_DIGEST_NAME_SHA2_256, "provider=default"));
    }

    OSSL_LIB_CTX* RsaSignature::SHA256::GetLib() const
    {
        return nullptr;
    }

    std::unique_ptr<OSSL_PARAM[]> RsaSignature::SHA256::GetParams() const
    {
        return nullptr;
    }

    RsaSignature::RsaSignature() : _ctx(Acore::Impl::GenericHashImpl::MakeCTX())
    {
    }

    RsaSignature::RsaSignature(RsaSignature const& other) : _ctx(Acore::Impl::GenericHashImpl::MakeCTX())
    {
        *this = other;
    }

    RsaSignature::RsaSignature(RsaSignature&& other) noexcept
    {
        *this = std::move(other);
    }

    RsaSignature::~RsaSignature()
    {
        EVP_MD_CTX_free(_ctx);
        EVP_PKEY_free(_key);
    }

    RsaSignature& RsaSignature::operator=(RsaSignature const& right)
    {
        if (this == &right)
            return *this;

        EVP_MD_CTX_copy_ex(_ctx, right._ctx);   // Allowed to fail if not yet initialized
        _key = right._key;                      // EVP_PKEY uses reference counting internally, just copy the pointer
        EVP_PKEY_up_ref(_key);                  // Bump reference count for PKEY, as every instance of this class holds two references to PKEY and destructor decrements it twice
        return *this;
    }

    RsaSignature& RsaSignature::operator=(RsaSignature&& right) noexcept
    {
        if (this == &right)
            return *this;

        _ctx = std::exchange(right._ctx, Acore::Impl::GenericHashImpl::MakeCTX());
        _key = std::exchange(right._key, EVP_PKEY_new());
        return *this;
    }

    bool RsaSignature::LoadKeyFromFile(std::string const& fileName)
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

    bool RsaSignature::LoadKeyFromString(std::string const& keyPem)
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

    bool RsaSignature::Sign(uint8 const* message, std::size_t messageLength, DigestGenerator& generator, std::vector<uint8>& output)
    {
        std::unique_ptr<EVP_MD, DigestGenerator::EVP_MD_Deleter> digestGenerator = generator.GetGenerator();

        auto keyCtx = make_unique_ptr_with_deleter<&EVP_PKEY_CTX_free>(EVP_PKEY_CTX_new_from_pkey(generator.GetLib(), _key, nullptr));
        EVP_MD_CTX_set_pkey_ctx(_ctx, keyCtx.get());

        std::unique_ptr<OSSL_PARAM[]> params = generator.GetParams();
        int result = EVP_DigestSignInit_ex(_ctx, nullptr, EVP_MD_get0_name(digestGenerator.get()), generator.GetLib(), nullptr, _key, params.get());

        if (result == 0)
            return false;

        result = EVP_DigestSignUpdate(_ctx, message, messageLength);
        if (result == 0)
            return false;

        std::size_t signatureLength = 0;
        result = EVP_DigestSignFinal(_ctx, nullptr, &signatureLength);
        if (result == 0)
            return false;

        output.resize(signatureLength);
        result = EVP_DigestSignFinal(_ctx, output.data(), &signatureLength);
        std::reverse(output.begin(), output.end());
        return result != 0;
    }
}
