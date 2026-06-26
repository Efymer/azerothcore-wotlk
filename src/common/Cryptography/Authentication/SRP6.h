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

#ifndef AZEROTHCORE_SRP6_H
#define AZEROTHCORE_SRP6_H

#include "AuthDefines.h"
#include "BigNumber.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "Optional.h"
#include <array>
#include <optional>
#include <span>
#include <vector>

namespace Acore::Crypto
{
    class AC_COMMON_API SRP6
    {
    public:
        static constexpr std::size_t SALT_LENGTH = 32;
        using Salt = std::array<uint8, SALT_LENGTH>;

        static constexpr std::size_t VERIFIER_LENGTH = 32;
        using Verifier = std::array<uint8, VERIFIER_LENGTH>;

        static constexpr std::size_t EPHEMERAL_KEY_LENGTH = 32;
        using EphemeralKey = std::array<uint8, EPHEMERAL_KEY_LENGTH>;

        static std::array<uint8, 1> const g;
        static std::array<uint8, 32> const N;

        // username + password must be passed through Utf8ToUpperOnlyLatin FIRST!
        static std::pair<Salt, Verifier> MakeRegistrationData(std::string const& username, std::string const& password);

        // username + password must be passed through Utf8ToUpperOnlyLatin FIRST!
        static bool CheckLogin(std::string const& username, std::string const& password, Salt const& salt, Verifier const& verifier)
        {
            return (verifier == CalculateVerifier(username, password, salt));
        }

        static SHA1::Digest GetSessionVerifier(EphemeralKey const& A, SHA1::Digest const& clientM, SessionKey const& K)
        {
            return SHA1::GetDigestOf(A, clientM, K);
        }

        SRP6(std::string const& username, Salt const& salt, Verifier const& verifier);
        std::optional<SessionKey> VerifyChallengeResponse(EphemeralKey const& A, SHA1::Digest const& clientM);

    private:
        bool _used = false; // a single instance can only be used to verify once

        static Verifier CalculateVerifier(std::string const& username, std::string const& password, Salt const& salt);
        static SessionKey SHA1Interleave(EphemeralKey const& S);

        /* global algorithm parameters */
        static BigNumber const _g; // a [g]enerator for the ring of integers mod N, algorithm parameter
        static BigNumber const _N; // the modulus, an algorithm parameter; all operations are mod this

        static EphemeralKey _B(BigNumber const& b, BigNumber const& v) { return ((_g.ModExp(b, _N) + (v * 3)) % N).ToByteArray<EPHEMERAL_KEY_LENGTH>(); }

        /* per-instantiation parameters, set on construction */
        SHA1::Digest const _I; // H(I) - the username, all uppercase
        BigNumber const _b; // b - randomly chosen by the server, 19 bytes, never given out
        BigNumber const _v; // v - the user's password verifier, derived from s + H(USERNAME || ":" || PASSWORD)

    public:
        Salt const s; // s - the user's password salt, random, used to calculate v on registration
        EphemeralKey const B; // B = 3v + g^b
    };

    namespace SRP
    {
        static constexpr std::size_t SALT_LENGTH = 32;
        using Salt = std::array<uint8, SALT_LENGTH>;

        using Verifier = std::vector<uint8>;

        // Battle.net SRP6 base. Self-contained (does not derive from the legacy
        // Acore::Crypto::SRP6 grunt implementation, which authserver still uses).
        class AC_COMMON_API BnetSRP6Base
        {
        protected:
            struct ForRegistrationTag { };

        public:
            explicit BnetSRP6Base(BigNumber const& i, Salt const& salt, Verifier const& verifier, BigNumber const& N, BigNumber const& g, BigNumber const& k);
            explicit BnetSRP6Base(ForRegistrationTag);

            BnetSRP6Base(BnetSRP6Base const&) = delete;
            BnetSRP6Base(BnetSRP6Base&&) = delete;
            BnetSRP6Base& operator=(BnetSRP6Base const&) = delete;
            BnetSRP6Base& operator=(BnetSRP6Base&&) = delete;

            virtual ~BnetSRP6Base() = default;

            virtual BigNumber const& GetN() const = 0;
            virtual BigNumber const& Getg() const = 0;

            virtual uint8 GetVersion() const = 0;
            virtual uint32 GetXIterations() const = 0;

            Optional<BigNumber> VerifyClientEvidence(BigNumber const& A, BigNumber const& clientM1);

            BigNumber CalculateServerEvidence(BigNumber const& A, BigNumber const& clientM1, BigNumber const& K) const;

            template<typename Impl>
            static std::pair<Salt, Verifier> MakeRegistrationData(std::string const& username, std::string const& password)
            {
                Impl impl(ForRegistrationTag{});
                return { impl.s, impl.CalculateVerifier(username, password, impl.s) };
            }

            bool CheckCredentials(std::string const& username, std::string const& password) const;

            Salt const s; // s - the user's password salt, random, used to calculate v on registration

        protected:
            static BigNumber CalculatePrivateB(BigNumber const& N);

            BigNumber CalculatePublicB(BigNumber const& N, BigNumber const& g, BigNumber const& k) const;

            virtual BigNumber CalculateX(std::string const& username, std::string const& password, Salt const& salt) const = 0;

            Verifier CalculateVerifier(std::string const& username, std::string const& password, Salt const& salt) const;

            virtual BigNumber CalculateU(BigNumber const& A) const = 0;

            virtual BigNumber DoCalculateEvidence(std::span<BigNumber const*> bns) const = 0;

            template<typename CryptoHash>
            BigNumber DoCalculateEvidence(std::span<BigNumber const*> bns) const
            {
                CryptoHash hash;
                for (BigNumber const* bn : bns)
                    hash.UpdateData(GetBrokenEvidenceVector(*bn));

                hash.Finalize();
                return BigNumber(hash.GetDigest(), false);
            }

            static std::vector<uint8> GetBrokenEvidenceVector(BigNumber const& bn);

            BigNumber const I; // H(I) - the username, all uppercase
            BigNumber const b; // b - randomly chosen by the server, same length as N, never given out
            BigNumber const v; // v - the user's password verifier, derived from s + H(USERNAME || ":" || PASSWORD)

        public:
            BigNumber const B; // B = k*v + g^b

        private:
            bool _used = false; // a single instance can only be used to verify once
        };

        class AC_COMMON_API BnetSRP6v1Base : public BnetSRP6Base
        {
        public:
            static BigNumber const N; // the modulus, an algorithm parameter; all operations are mod this
            static BigNumber const g; // a [g]enerator for the ring of integers mod N, algorithm parameter

            explicit BnetSRP6v1Base(std::string const& username, Salt const& salt, Verifier const& verifier, BigNumber const& k);
            explicit BnetSRP6v1Base(ForRegistrationTag t) : BnetSRP6Base(t) { }

            BigNumber const& GetN() const final { return N; }
            BigNumber const& Getg() const final { return g; }

            uint8 GetVersion() const final { return 1; }
            uint32 GetXIterations() const final { return 1; }

        protected:
            BigNumber CalculateX(std::string const& username, std::string const& password, Salt const& salt) const final;
        };

        class AC_COMMON_API BnetSRP6v2Base : public BnetSRP6Base
        {
        public:
            static BigNumber const N; // the modulus, an algorithm parameter; all operations are mod this
            static BigNumber const g; // a [g]enerator for the ring of integers mod N, algorithm parameter

            explicit BnetSRP6v2Base(std::string const& username, Salt const& salt, Verifier const& verifier, BigNumber const& k);
            explicit BnetSRP6v2Base(ForRegistrationTag t) : BnetSRP6Base(t) { }

            BigNumber const& GetN() const final { return N; }
            BigNumber const& Getg() const final { return g; }

            uint8 GetVersion() const final { return 2; }
            uint32 GetXIterations() const final { return 15000; }

        protected:
            BigNumber CalculateX(std::string const& username, std::string const& password, Salt const& salt) const final;
        };

        template<typename CryptoHash>
        class BnetSRP6v1 final : public BnetSRP6v1Base
        {
        public:
            BnetSRP6v1(std::string const& username, Salt const& salt, Verifier const& verifier)
                : BnetSRP6v1Base(username, salt, verifier, BigNumber(CryptoHash::GetDigestOf(N.ToByteArray<128>(false), g.ToByteArray<128>(false)), false))
            {
            }

            explicit BnetSRP6v1(ForRegistrationTag t) : BnetSRP6v1Base(t) { }

        protected:
            BigNumber CalculateU(BigNumber const& A) const override
            {
                return BigNumber(CryptoHash::GetDigestOf(A.ToByteArray<128>(false), B.ToByteArray<128>(false)), false);
            }

            BigNumber DoCalculateEvidence(std::span<BigNumber const*> bns) const override
            {
                return BnetSRP6Base::DoCalculateEvidence<CryptoHash>(bns);
            }
        };

        template<typename CryptoHash>
        class BnetSRP6v2 final : public BnetSRP6v2Base
        {
        public:
            BnetSRP6v2(std::string const& username, Salt const& salt, Verifier const& verifier)
                : BnetSRP6v2Base(username, salt, verifier, BigNumber(CryptoHash::GetDigestOf(N.ToByteArray<256>(false), g.ToByteArray<256>(false)), false))
            {
            }

            explicit BnetSRP6v2(ForRegistrationTag t) : BnetSRP6v2Base(t) { }

        protected:
            BigNumber CalculateU(BigNumber const& A) const override
            {
                return BigNumber(CryptoHash::GetDigestOf(A.ToByteArray<256>(false), B.ToByteArray<256>(false)), false);
            }

            BigNumber DoCalculateEvidence(std::span<BigNumber const*> bns) const override
            {
                return BnetSRP6Base::DoCalculateEvidence<CryptoHash>(bns);
            }
        };
    }
}

#endif
