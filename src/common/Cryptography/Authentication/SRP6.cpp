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

#include "SRP6.h"
#include "CryptoRandom.h"
#include "Util.h"
#include <algorithm>
#include <functional>
#include <openssl/evp.h>

using SHA1 = Acore::Crypto::SHA1;
using SRP6 = Acore::Crypto::SRP6;

/*static*/ std::array<uint8, 1> const SRP6::g = { 7 };
/*static*/ std::array<uint8, 32> const SRP6::N = HexStrToByteArray<32>("894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7", true);
/*static*/ BigNumber const SRP6::_g(SRP6::g);
/*static*/ BigNumber const SRP6::_N(N);

/*static*/ std::pair<SRP6::Salt, SRP6::Verifier> SRP6::MakeRegistrationData(std::string const& username, std::string const& password)
{
    std::pair<SRP6::Salt, SRP6::Verifier> res;
    Crypto::GetRandomBytes(res.first); // random salt
    res.second = CalculateVerifier(username, password, res.first);
    return res;
}

/*static*/ SRP6::Verifier SRP6::CalculateVerifier(std::string const& username, std::string const& password, SRP6::Salt const& salt)
{
    // v = g ^ H(s || H(u || ':' || p)) mod N
    return _g.ModExp(
        SHA1::GetDigestOf(
            salt,
            SHA1::GetDigestOf(username, ":", password)
        )
    ,_N).ToByteArray<32>();
}

/*static*/ SessionKey SRP6::SHA1Interleave(SRP6::EphemeralKey const& S)
{
    // split S into two buffers
    std::array<uint8, EPHEMERAL_KEY_LENGTH / 2> buf0{}, buf1{};
    for (std::size_t i = 0; i < EPHEMERAL_KEY_LENGTH / 2; ++i)
    {
        buf0[i] = S[2 * i + 0];
        buf1[i] = S[2 * i + 1];
    }

    // find position of first nonzero byte
    std::size_t p = 0;
    while (p < EPHEMERAL_KEY_LENGTH && !S[p])
        ++p;

    if (p & 1)
        ++p; // skip one extra byte if p is odd

    p /= 2; // offset into buffers

    // hash each of the halves, starting at the first nonzero byte
    SHA1::Digest const hash0 = SHA1::GetDigestOf(buf0.data() + p, EPHEMERAL_KEY_LENGTH / 2 - p);
    SHA1::Digest const hash1 = SHA1::GetDigestOf(buf1.data() + p, EPHEMERAL_KEY_LENGTH / 2 - p);

    // stick the two hashes back together
    SessionKey K;
    for (std::size_t i = 0; i < SHA1::DIGEST_LENGTH; ++i)
    {
        K[2 * i + 0] = hash0[i];
        K[2 * i + 1] = hash1[i];
    }
    return K;
}

SRP6::SRP6(std::string const& username, Salt const& salt, Verifier const& verifier)
    : _I(SHA1::GetDigestOf(username)), _b(Crypto::GetRandomBytes<32>()), _v(verifier), s(salt), B(_B(_b, _v)) {}

std::optional<SessionKey> SRP6::VerifyChallengeResponse(EphemeralKey const& A, SHA1::Digest const& clientM)
{
    ASSERT(!_used, "A single SRP6 object must only ever be used to verify ONCE!");
    _used = true;

    BigNumber const _A(A);
    if ((_A % _N).IsZero())
        return std::nullopt;

    BigNumber const u(SHA1::GetDigestOf(A, B));
    EphemeralKey const S = (_A * (_v.ModExp(u, _N))).ModExp(_b, N).ToByteArray<32>();

    SessionKey K = SHA1Interleave(S);

    // NgHash = H(N) xor H(g)
    SHA1::Digest const NHash = SHA1::GetDigestOf(N);
    SHA1::Digest const gHash = SHA1::GetDigestOf(g);
    SHA1::Digest NgHash;
    std::transform(NHash.begin(), NHash.end(), gHash.begin(), NgHash.begin(), std::bit_xor<>());

    SHA1::Digest const ourM = SHA1::GetDigestOf(NgHash, _I, s, A, B, K);
    if (ourM == clientM)
        return K;

    return std::nullopt;
}

namespace Acore::Crypto::SRP
{
    using SHA256 = Acore::Crypto::SHA256;
    using SHA512 = Acore::Crypto::SHA512;

    using namespace std::string_literals;

    namespace
    {
        BigNumber VerifierToBigNumber(Verifier const& verifier)
        {
            BigNumber bn;
            bn.SetBinary(verifier);
            return bn;
        }
    }

    BigNumber const BnetSRP6v1Base::N = "86A7F6DEEB306CE519770FE37D556F29944132554DED0BD68205E27F3231FEF5A10108238A3150C59CAF7B0B6478691C13A6ACF5E1B5ADAFD4A943D4A21A142B800E8A55F8BFBAC700EB77A7235EE5A609E350EA9FC19F10D921C2FA832E4461B7125D38D254A0BE873DFC27858ACB3F8B9F258461E4373BC3A6C2A9634324AB"s;
    BigNumber const BnetSRP6v1Base::g = 2;

    BigNumber const BnetSRP6v2Base::N = "AC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73"s;
    BigNumber const BnetSRP6v2Base::g = 2;

    BnetSRP6Base::BnetSRP6Base(BigNumber const& i, Salt const& salt, Verifier const& verifier, BigNumber const& N, BigNumber const& g, BigNumber const& k)
        : s(salt), I(i), b(CalculatePrivateB(N)), v(VerifierToBigNumber(verifier)), B(CalculatePublicB(N, g, k))
    {
    }

    BnetSRP6Base::BnetSRP6Base(ForRegistrationTag)
        : s(Acore::Crypto::GetRandomBytes<SALT_LENGTH>()), _used(true)
    {
    }

    Optional<BigNumber> BnetSRP6Base::VerifyClientEvidence(BigNumber const& A, BigNumber const& clientM1)
    {
        ASSERT(!_used, "A single SRP6 object must only ever be used to verify ONCE!");
        _used = true;

        BigNumber N = GetN();
        if ((A % N).IsZero())
            return {};

        BigNumber const u = CalculateU(A);
        if ((u % N).IsZero())
            return {};

        BigNumber const S = (A * v.ModExp(u, N)).ModExp(b, N);

        std::array evidenceBns{ &A, &B, &S };
        BigNumber const ourM = DoCalculateEvidence(evidenceBns);
        if (ourM != clientM1)
            return {};

        return S;
    }

    BigNumber BnetSRP6Base::CalculateServerEvidence(BigNumber const& A, BigNumber const& clientM1, BigNumber const& K) const
    {
        std::array evidenceBns{ &A, &clientM1, &K };
        return DoCalculateEvidence(evidenceBns);
    }

    bool BnetSRP6Base::CheckCredentials(std::string const& username, std::string const& password) const
    {
        Verifier const computed = CalculateVerifier(username, password, s);
        BigNumber computedBn;
        computedBn.SetBinary(computed);
        return v == computedBn;
    }

    BigNumber BnetSRP6Base::CalculatePrivateB(BigNumber const& N)
    {
        BigNumber b;
        b.SetRand(N.GetNumBits());
        b %= N - 1;
        return b;
    }

    BigNumber BnetSRP6Base::CalculatePublicB(BigNumber const& N, BigNumber const& g, BigNumber const& k) const
    {
        return (g.ModExp(b, N) + (v * k)) % N;
    }

    Verifier BnetSRP6Base::CalculateVerifier(std::string const& username, std::string const& password, Salt const& salt) const
    {
        // v = g ^ H(s || H(u || ':' || p)) mod N
        return Getg().ModExp(
            CalculateX(username, password, salt),
            GetN()
        ).ToByteVector();
    }

    std::vector<uint8> BnetSRP6Base::GetBrokenEvidenceVector(BigNumber const& bn)
    {
        int32 bytes = (bn.GetNumBits() + 8) >> 3;
        return bn.ToByteVector(bytes, false);
    }

    BnetSRP6v1Base::BnetSRP6v1Base(std::string const& username, Salt const& salt, Verifier const& verifier, BigNumber const& k)
        : BnetSRP6Base(SHA256::GetDigestOf(username), salt, verifier, N, g, k)
    {
    }

    BigNumber BnetSRP6v1Base::CalculateX(std::string const& username, std::string const& password, Salt const& salt) const
    {
        return SHA256::GetDigestOf(
            salt,
            SHA256::GetDigestOf(username, ":", password)
        );
    }

    BnetSRP6v2Base::BnetSRP6v2Base(std::string const& username, Salt const& salt, Verifier const& verifier, BigNumber const& k)
        : BnetSRP6Base(SHA256::GetDigestOf(username), salt, verifier, N, g, k)
    {
    }

    BigNumber BnetSRP6v2Base::CalculateX(std::string const& username, std::string const& password, Salt const& salt) const
    {
        SHA512::Digest xBytes = {};
        std::string tmp = username + ":" + password;
        PKCS5_PBKDF2_HMAC(tmp.c_str(), tmp.length(), salt.data(), salt.size(), GetXIterations(), EVP_sha512(), xBytes.size(), xBytes.data());
        BigNumber x(xBytes, false);
        if (xBytes[0] & 0x80)
        {
            std::array<uint8, 65> fix = {};
            fix[64] = 1;
            x -= fix;
        }

        return x % (N - 1);
    }
}
