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

#ifndef AZEROTHCORE_RSA_H
#define AZEROTHCORE_RSA_H

#include "Define.h"
#include <openssl/evp.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace Acore::Crypto
{
    class AC_COMMON_API RsaSignature
    {
    public:
        class AC_COMMON_API DigestGenerator
        {
        public:
            struct EVP_MD_Deleter
            {
                void operator()(EVP_MD* md) const;
            };

            virtual ~DigestGenerator() = default;
            virtual std::unique_ptr<EVP_MD, EVP_MD_Deleter> GetGenerator() const = 0;

            virtual OSSL_LIB_CTX* GetLib() const = 0;
            virtual std::unique_ptr<OSSL_PARAM[]> GetParams() const = 0;
        };

        class AC_COMMON_API SHA256 : public DigestGenerator
        {
        public:
            std::unique_ptr<EVP_MD, EVP_MD_Deleter> GetGenerator() const override;

            OSSL_LIB_CTX* GetLib() const override;
            std::unique_ptr<OSSL_PARAM[]> GetParams() const override;
        };

        RsaSignature();
        RsaSignature(RsaSignature const& other);
        RsaSignature(RsaSignature&& other) noexcept;
        ~RsaSignature();

        RsaSignature& operator=(RsaSignature const& right);
        RsaSignature& operator=(RsaSignature&& right) noexcept;

        bool LoadKeyFromFile(std::string const& fileName);

        bool LoadKeyFromString(std::string const& keyPem);

        template <std::size_t N>
        bool Sign(std::array<uint8, N> const& message, DigestGenerator& generator, std::vector<uint8>& output)
        {
            return this->Sign(message.data(), message.size(), generator, output);
        }

        bool Sign(uint8 const* message, std::size_t messageLength, DigestGenerator& generator, std::vector<uint8>& output);

    private:
        EVP_MD_CTX* _ctx = nullptr;
        EVP_PKEY* _key = nullptr;
    };
}

#endif // AZEROTHCORE_RSA_H
