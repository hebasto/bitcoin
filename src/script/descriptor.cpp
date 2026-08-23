// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <crypto/hex_base.h>
#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <musig.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/expected.h>
#include <util/string.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using util::Split;

namespace {

////////////////////////////////////////////////////////////////////////////
// Checksum                                                               //
////////////////////////////////////////////////////////////////////////////

// This section implements a checksum algorithm for descriptors with the
// following properties:
// * Mistakes in a descriptor string are measured in "symbol errors". The higher
//   the number of symbol errors, the harder it is to detect:
//   * An error substituting a character from 0123456789()[],'/*abcdefgh@:$%{} for
//     another in that set always counts as 1 symbol error.
//     * Note that hex encoded keys are covered by these characters. Xprvs and
//       xpubs use other characters too, but already have their own checksum
//       mechanism.
//     * Function names like "multi()" use other characters, but mistakes in
//       these would generally result in an unparsable descriptor.
//   * A case error always counts as 1 symbol error.
//   * Any other 1 character substitution error counts as 1 or 2 symbol errors.
// * Any 1 symbol error is always detected.
// * Any 2 or 3 symbol error in a descriptor of up to 49154 characters is always detected.
// * Any 4 symbol error in a descriptor of up to 507 characters is always detected.
// * Any 5 symbol error in a descriptor of up to 77 characters is always detected.
// * Is optimized to minimize the chance a 5 symbol error in a descriptor up to 387 characters is undetected
// * Random errors have a chance of 1 in 2**40 of being undetected.
//
// These properties are achieved by expanding every group of 3 (non checksum) characters into
// 4 GF(32) symbols, over which a cyclic code is defined.

/*
 * Interprets c as 8 groups of 5 bits which are the coefficients of a degree 8 polynomial over GF(32),
 * multiplies that polynomial by x, computes its remainder modulo a generator, and adds the constant term val.
 *
 * This generator is G(x) = x^8 + {30}x^7 + {23}x^6 + {15}x^5 + {14}x^4 + {10}x^3 + {6}x^2 + {12}x + {9}.
 * It is chosen to define an cyclic error detecting code which is selected by:
 * - Starting from all BCH codes over GF(32) of degree 8 and below, which by construction guarantee detecting
 *   3 errors in windows up to 19000 symbols.
 * - Taking all those generators, and for degree 7 ones, extend them to degree 8 by adding all degree-1 factors.
 * - Selecting just the set of generators that guarantee detecting 4 errors in a window of length 512.
 * - Selecting one of those with best worst-case behavior for 5 errors in windows of length up to 512.
 *
 * The generator and the constants to implement it can be verified using this Sage code:
 *   B = GF(2) # Binary field
 *   BP.<b> = B[] # Polynomials over the binary field
 *   F_mod = b**5 + b**3 + 1
 *   F.<f> = GF(32, modulus=F_mod, repr='int') # GF(32) definition
 *   FP.<x> = F[] # Polynomials over GF(32)
 *   E_mod = x**3 + x + F.fetch_int(8)
 *   E.<e> = F.extension(E_mod) # Extension field definition
 *   alpha = e**2743 # Choice of an element in extension field
 *   for p in divisors(E.order() - 1): # Verify alpha has order 32767.
 *       assert((alpha**p == 1) == (p % 32767 == 0))
 *   G = lcm([(alpha**i).minpoly() for i in [1056,1057,1058]] + [x + 1])
 *   print(G) # Print out the generator
 *   for i in [1,2,4,8,16]: # Print out {1,2,4,8,16}*(G mod x^8), packed in hex integers.
 *       v = 0
 *       for coef in reversed((F.fetch_int(i)*(G % x**8)).coefficients(sparse=True)):
 *           v = v*32 + coef.integer_representation()
 *       print("0x%x" % v)
 */
uint64_t PolyMod(uint64_t c, int val)
{
    uint8_t c0 = c >> 35;
    c = ((c & 0x7ffffffff) << 5) ^ val;
    if (c0 & 1) c ^= 0xf5dee51989;
    if (c0 & 2) c ^= 0xa9fdca3312;
    if (c0 & 4) c ^= 0x1bab10e32d;
    if (c0 & 8) c ^= 0x3706b1677a;
    if (c0 & 16) c ^= 0x644d626ffd;
    return c;
}

std::string DescriptorChecksum(const std::span<const char>& span)
{
    /** A character set designed such that:
     *  - The most common 'unprotected' descriptor characters (hex, keypaths) are in the first group of 32.
     *  - Case errors cause an offset that's a multiple of 32.
     *  - As many alphabetic characters are in the same group (while following the above restrictions).
     *
     * If p(x) gives the position of a character c in this character set, every group of 3 characters
     * (a,b,c) is encoded as the 4 symbols (p(a) & 31, p(b) & 31, p(c) & 31, (p(a) / 32) + 3 * (p(b) / 32) + 9 * (p(c) / 32).
     * This means that changes that only affect the lower 5 bits of the position, or only the higher 2 bits, will just
     * affect a single symbol.
     *
     * As a result, within-group-of-32 errors count as 1 symbol, as do cross-group errors that don't affect
     * the position within the groups.
     */
    static const std::string INPUT_CHARSET =
        "0123456789()[],'/*abcdefgh@:$%{}"
        "IJKLMNOPQRSTUVWXYZ&+-.;<=>?!^_|~"
        "ijklmnopqrstuvwxyzABCDEFGH`#\"\\ ";

    /** The character set for the checksum itself (same as bech32). */
    static const std::string CHECKSUM_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    uint64_t c = 1;
    int cls = 0;
    int clscount = 0;
    for (auto ch : span) {
        auto pos = INPUT_CHARSET.find(ch);
        if (pos == std::string::npos) return "";
        c = PolyMod(c, pos & 31); // Emit a symbol for the position inside the group, for every character.
        cls = cls * 3 + (pos >> 5); // Accumulate the group numbers
        if (++clscount == 3) {
            // Emit an extra symbol representing the group numbers, for every 3 characters.
            c = PolyMod(c, cls);
            cls = 0;
            clscount = 0;
        }
    }
    if (clscount > 0) c = PolyMod(c, cls);
    for (int j = 0; j < 8; ++j) c = PolyMod(c, 0); // Shift further to determine the checksum.
    c ^= 1; // Prevent appending zeroes from not affecting the checksum.

    std::string ret(8, ' ');
    for (int j = 0; j < 8; ++j) ret[j] = CHECKSUM_CHARSET[(c >> (5 * (7 - j))) & 31];
    return ret;
}

std::string AddChecksum(const std::string& str) { return str + "#" + DescriptorChecksum(str); }

////////////////////////////////////////////////////////////////////////////
// Internal representation                                                //
////////////////////////////////////////////////////////////////////////////

typedef std::vector<uint32_t> KeyPath;

/** Interface for public key objects in descriptors. */
struct PubkeyProvider
{
public:
    //! Index of this key expression in the descriptor
    //! E.g. If this PubkeyProvider is key1 in multi(2, key1, key2, key3), then m_expr_index = 0
    const uint32_t m_expr_index;

    explicit PubkeyProvider(uint32_t exp_index) : m_expr_index(exp_index) {}

    virtual ~PubkeyProvider() = default;

    /** Compare two public keys represented by this provider.
     * Used by the Miniscript descriptors to check for duplicate keys in the script.
     */
    bool operator<(PubkeyProvider& other) const {
        FlatSigningProvider dummy;

        std::optional<CPubKey> a = GetPubKey(0, dummy, dummy);
        std::optional<CPubKey> b = other.GetPubKey(0, dummy, dummy);

        return a < b;
    }

    /** Derive a public key and put it into out.
     *  read_cache is the cache to read keys from (if not nullptr)
     *  write_cache is the cache to write keys to (if not nullptr)
     *  Caches are not exclusive but this is not tested. Currently we use them exclusively
     */
    virtual std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const = 0;

    /** Whether this represent multiple public keys at different positions. */
    virtual bool IsRange() const = 0;

    /** Get the size of the generated public key(s) in bytes (33 or 65). */
    virtual size_t GetSize() const = 0;

    enum class StringType {
        PUBLIC,
        COMPAT // string calculation that mustn't change over time to stay compatible with previous software versions
    };

    /** Get the descriptor string form. */
    virtual std::string ToString(StringType type=StringType::PUBLIC) const = 0;

    /** Get the descriptor string form including private data (if available in arg).
     *  If the private data is not available, the output string in the "out" parameter
     *  will not contain any private key information,
     *  and this function will return "false".
     */
    virtual bool ToPrivateString(const SigningProvider& arg, std::string& out) const = 0;

    /** Get the descriptor string form with the xpub at the last hardened derivation,
     *  and always use h for hardened derivation.
     */
    virtual bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache = nullptr) const = 0;

    /** Derive a private key, if private data is available in arg and put it into out. */
    virtual void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const = 0;

    /** Whether private data for this provider is available in arg. */
    virtual bool HavePrivateKeys(const SigningProvider& arg) const
    {
        FlatSigningProvider tmp_provider;
        GetPrivKey(/*pos=*/0, arg, tmp_provider);
        return !tmp_provider.keys.empty();
    }

    /** Return the non-extended public key for this PubkeyProvider, if it has one. */
    virtual std::optional<CPubKey> GetRootPubKey() const = 0;
    /** Return the extended public key for this PubkeyProvider, if it has one. */
    virtual std::optional<CExtPubKey> GetRootExtPubKey() const = 0;

    /** Make a deep copy of this PubkeyProvider */
    virtual std::unique_ptr<PubkeyProvider> Clone() const = 0;

    /** Whether this PubkeyProvider is a BIP 32 extended key that can be derived from */
    virtual bool IsBIP32() const = 0;

    /** Get the count of keys known by this PubkeyProvider. Usually one, but may be more for key aggregation schemes */
    virtual size_t GetKeyCount() const { return 1; }

    /** Whether this PubkeyProvider can always provide a public key without cache or private key arguments */
    virtual bool CanSelfExpand() const = 0;
};

class OriginPubkeyProvider final : public PubkeyProvider
{
    KeyOriginInfo m_origin;
    std::unique_ptr<PubkeyProvider> m_provider;
    bool m_apostrophe;

    std::string OriginString(StringType type, bool normalized=false) const
    {
        // If StringType==COMPAT, always use the apostrophe to stay compatible with previous versions
        bool use_apostrophe = (!normalized && m_apostrophe) || type == StringType::COMPAT;
        return HexStr(m_origin.fingerprint) + FormatHDKeypath(m_origin.path, use_apostrophe);
    }

public:
    OriginPubkeyProvider(uint32_t exp_index, KeyOriginInfo info, std::unique_ptr<PubkeyProvider> provider, bool apostrophe) : PubkeyProvider(exp_index), m_origin(std::move(info)), m_provider(std::move(provider)), m_apostrophe(apostrophe) {}
    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        std::optional<CPubKey> pub = m_provider->GetPubKey(pos, arg, out, read_cache, write_cache);
        if (!pub) return std::nullopt;
        Assert(out.pubkeys.contains(pub->GetID()));
        auto& [pubkey, suborigin] = out.origins[pub->GetID()];
        Assert(pubkey == *pub); // m_provider must have a valid origin by this point.
        suborigin.fingerprint = m_origin.fingerprint;
        suborigin.path.insert(suborigin.path.begin(), m_origin.path.begin(), m_origin.path.end());
        return pub;
    }
    bool IsRange() const override { return m_provider->IsRange(); }
    size_t GetSize() const override { return m_provider->GetSize(); }
    bool IsBIP32() const override { return m_provider->IsBIP32(); }
    std::string ToString(StringType type) const override { return "[" + OriginString(type) + "]" + m_provider->ToString(type); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        std::string sub;
        bool has_priv_key{m_provider->ToPrivateString(arg, sub)};
        ret = "[" + OriginString(StringType::PUBLIC) + "]" + std::move(sub);
        return has_priv_key;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& ret, const DescriptorCache* cache) const override
    {
        std::string sub;
        if (!m_provider->ToNormalizedString(arg, sub, cache)) return false;
        // If m_provider is a BIP32PubkeyProvider, we may get a string formatted like a OriginPubkeyProvider
        // In that case, we need to strip out the leading square bracket and fingerprint from the substring,
        // and append that to our own origin string.
        if (sub[0] == '[') {
            sub = sub.substr(9);
            ret = "[" + OriginString(StringType::PUBLIC, /*normalized=*/true) + std::move(sub);
        } else {
            ret = "[" + OriginString(StringType::PUBLIC, /*normalized=*/true) + "]" + std::move(sub);
        }
        return true;
    }
    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        m_provider->GetPrivKey(pos, arg, out);
    }
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return m_provider->GetRootPubKey();
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return m_provider->GetRootExtPubKey();
    }
    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        return std::make_unique<OriginPubkeyProvider>(m_expr_index, m_origin, m_provider->Clone(), m_apostrophe);
    }
    bool CanSelfExpand() const override { return m_provider->CanSelfExpand(); }
};

/** An object representing a parsed constant public key in a descriptor. */
class ConstPubkeyProvider final : public PubkeyProvider
{
    CPubKey m_pubkey;
    bool m_xonly;

    std::optional<CKey> GetPrivKey(const SigningProvider& arg) const
    {
        CKey key;
        if (!(m_xonly ? arg.GetKeyByXOnly(XOnlyPubKey(m_pubkey), key) :
                        arg.GetKey(m_pubkey.GetID(), key))) return std::nullopt;
        return key;
    }

public:
    ConstPubkeyProvider(uint32_t exp_index, const CPubKey& pubkey, bool xonly) : PubkeyProvider(exp_index), m_pubkey(pubkey), m_xonly(xonly) {}
    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider&, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        KeyOriginInfo info;
        CKeyID keyid = m_pubkey.GetID();
        info.fingerprint = keyid.fingerprint();
        out.origins.emplace(keyid, std::make_pair(m_pubkey, info));
        out.pubkeys.emplace(keyid, m_pubkey);
        return m_pubkey;
    }
    bool IsRange() const override { return false; }
    size_t GetSize() const override { return m_pubkey.size(); }
    bool IsBIP32() const override { return false; }
    std::string ToString(StringType type) const override { return m_xonly ? HexStr(m_pubkey).substr(2) : HexStr(m_pubkey); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        std::optional<CKey> key = GetPrivKey(arg);
        if (!key) {
            ret = ToString(StringType::PUBLIC);
            return false;
        }
        ret = EncodeSecret(*key);
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& ret, const DescriptorCache* cache) const override
    {
        ret = ToString(StringType::PUBLIC);
        return true;
    }
    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        std::optional<CKey> key = GetPrivKey(arg);
        if (!key) return;
        out.keys.emplace(key->GetPubKey().GetID(), *key);
    }
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return m_pubkey;
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return std::nullopt;
    }
    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        return std::make_unique<ConstPubkeyProvider>(m_expr_index, m_pubkey, m_xonly);
    }
    bool CanSelfExpand() const final { return true; }
};

enum class DeriveType {
    NON_RANGED,
    UNHARDENED_RANGED,
    HARDENED_RANGED,
};

/** An object representing a parsed extended public key in a descriptor. */
class BIP32PubkeyProvider final : public PubkeyProvider
{
    // Root xpub, path, and final derivation step type being used, if any
    CExtPubKey m_root_extkey;
    KeyPath m_path;
    DeriveType m_derive;
    // Whether ' or h is used in harded derivation
    bool m_apostrophe;

    bool GetExtKey(const SigningProvider& arg, CExtKey& ret) const
    {
        CKey key;
        if (!arg.GetKey(m_root_extkey.pubkey.GetID(), key)) return false;
        ret.nDepth = m_root_extkey.nDepth;
        ret.fingerprint = m_root_extkey.fingerprint;
        ret.nChild = m_root_extkey.nChild;
        ret.chaincode = m_root_extkey.chaincode;
        ret.key = key;
        return true;
    }

    // Derives the last xprv
    bool GetDerivedExtKey(const SigningProvider& arg, CExtKey& xprv, CExtKey& last_hardened) const
    {
        if (!GetExtKey(arg, xprv)) return false;
        for (auto entry : m_path) {
            if (!xprv.Derive(xprv, entry)) return false;
            if (entry >> 31) {
                last_hardened = xprv;
            }
        }
        return true;
    }

    bool IsHardened() const
    {
        if (m_derive == DeriveType::HARDENED_RANGED) return true;
        return HasHardenedDerivation(m_path);
    }

public:
    BIP32PubkeyProvider(uint32_t exp_index, const CExtPubKey& extkey, KeyPath path, DeriveType derive, bool apostrophe) : PubkeyProvider(exp_index), m_root_extkey(extkey), m_path(std::move(path)), m_derive(derive), m_apostrophe(apostrophe) {}
    bool IsRange() const override { return m_derive != DeriveType::NON_RANGED; }
    size_t GetSize() const override { return 33; }
    bool IsBIP32() const override { return true; }
    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        CExtPubKey final_extkey = m_root_extkey;
        return final_extkey.pubkey;
    }
    std::string ToString(StringType type, bool normalized) const
    {
        // If StringType==COMPAT, always use the apostrophe to stay compatible with previous versions
        const bool use_apostrophe = (!normalized && m_apostrophe) || type == StringType::COMPAT;
        std::string ret = EncodeExtPubKey(m_root_extkey) + FormatHDKeypath(m_path, /*apostrophe=*/use_apostrophe);
        if (IsRange()) {
            ret += "/*";
            if (m_derive == DeriveType::HARDENED_RANGED) ret += use_apostrophe ? '\'' : 'h';
        }
        return ret;
    }
    std::string ToString(StringType type=StringType::PUBLIC) const override
    {
        return ToString(type, /*normalized=*/false);
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        CExtKey key;
        if (!GetExtKey(arg, key)) {
            out = ToString(StringType::PUBLIC);
            return false;
        }
        out = EncodeExtKey(key) + FormatHDKeypath(m_path, /*apostrophe=*/m_apostrophe);
        if (IsRange()) {
            out += "/*";
            if (m_derive == DeriveType::HARDENED_RANGED) out += m_apostrophe ? '\'' : 'h';
        }
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache) const override
    {
        return true;
    }
    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        CExtKey extkey;
        CExtKey dummy;
        if (!GetDerivedExtKey(arg, extkey, dummy)) return;
        if (m_derive == DeriveType::UNHARDENED_RANGED && !extkey.Derive(extkey, pos)) return;
        if (m_derive == DeriveType::HARDENED_RANGED && !extkey.Derive(extkey, pos | BIP32_HARDENED_FLAG)) return;
        out.keys.emplace(extkey.key.GetPubKey().GetID(), extkey.key);
    }
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return std::nullopt;
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return m_root_extkey;
    }
    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        return std::make_unique<BIP32PubkeyProvider>(m_expr_index, m_root_extkey, m_path, m_derive, m_apostrophe);
    }
    bool CanSelfExpand() const override { return !IsHardened(); }
};

/** PubkeyProvider for a musig() expression */
class MuSigPubkeyProvider final : public PubkeyProvider
{
private:
    //! PubkeyProvider for the participants
    const std::vector<std::unique_ptr<PubkeyProvider>> m_participants;
    //! Derivation path
    const KeyPath m_path;
    //! PubkeyProvider for the aggregate pubkey if it can be cached (i.e. participants are not ranged)
    mutable std::unique_ptr<PubkeyProvider> m_aggregate_provider;
    mutable std::optional<CPubKey> m_aggregate_pubkey;
    const DeriveType m_derive;
    const bool m_ranged_participants;

    bool IsRangedDerivation() const { return m_derive != DeriveType::NON_RANGED; }

public:
    MuSigPubkeyProvider(
        uint32_t exp_index,
        std::vector<std::unique_ptr<PubkeyProvider>> providers,
        KeyPath path,
        DeriveType derive
    )
        : PubkeyProvider(exp_index),
        m_participants(std::move(providers)),
        m_path(std::move(path)),
        m_derive(derive),
        m_ranged_participants(std::any_of(m_participants.begin(), m_participants.end(), [](const auto& pubkey) { return pubkey->IsRange(); }))
    {
        if (!Assume(!(m_ranged_participants && IsRangedDerivation()))) {
            throw std::runtime_error("musig(): Cannot have both ranged participants and ranged derivation");
        }
        if (!Assume(m_derive != DeriveType::HARDENED_RANGED)) {
            throw std::runtime_error("musig(): Cannot have hardened derivation");
        }
    }

    std::optional<CPubKey> GetPubKey(int pos, const SigningProvider& arg, FlatSigningProvider& out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        FlatSigningProvider dummy;
        // If the participants are not ranged, we can compute and cache the aggregate pubkey by creating a PubkeyProvider for it
        if (!m_aggregate_provider && !m_ranged_participants) {
            // Retrieve the pubkeys from the providers
            std::vector<CPubKey> pubkeys;
            for (const auto& prov : m_participants) {
                std::optional<CPubKey> pubkey = prov->GetPubKey(0, arg, dummy, read_cache, write_cache);
                if (!pubkey.has_value()) {
                    return std::nullopt;
                }
                pubkeys.push_back(pubkey.value());
            }
            std::sort(pubkeys.begin(), pubkeys.end());

            // Aggregate the pubkey
            m_aggregate_pubkey = MuSig2AggregatePubkeys(pubkeys);
            if (!Assume(m_aggregate_pubkey.has_value())) return std::nullopt;

            // Make our pubkey provider
            if (IsRangedDerivation() || !m_path.empty()) {
                // Make the synthetic xpub and construct the BIP32PubkeyProvider
                CExtPubKey extpub = CreateMuSig2SyntheticXpub(m_aggregate_pubkey.value());
                m_aggregate_provider = std::make_unique<BIP32PubkeyProvider>(m_expr_index, extpub, m_path, m_derive, /*apostrophe=*/false);
            } else {
                m_aggregate_provider = std::make_unique<ConstPubkeyProvider>(m_expr_index, m_aggregate_pubkey.value(), /*xonly=*/false);
            }
        }

        // Retrieve all participant pubkeys
        std::vector<CPubKey> pubkeys;
        for (const auto& prov : m_participants) {
            std::optional<CPubKey> pub = prov->GetPubKey(pos, arg, out, read_cache, write_cache);
            if (!pub) return std::nullopt;
            pubkeys.emplace_back(*pub);
        }
        std::sort(pubkeys.begin(), pubkeys.end());

        CPubKey pubout;
        if (m_aggregate_provider) {
            // When we have a cached aggregate key, we are either returning it or deriving from it
            // Either way, we can passthrough to its GetPubKey
            // Use a dummy signing provider as private keys do not exist for the aggregate pubkey
            std::optional<CPubKey> pub = m_aggregate_provider->GetPubKey(pos, dummy, out, read_cache, write_cache);
            if (!pub) return std::nullopt;
            pubout = *pub;
            out.aggregate_pubkeys.emplace(m_aggregate_pubkey.value(), pubkeys);
        } else {
            if (!Assume(m_ranged_participants) || !Assume(m_path.empty())) return std::nullopt;
            // Compute aggregate key from derived participants
            std::optional<CPubKey> aggregate_pubkey = MuSig2AggregatePubkeys(pubkeys);
            if (!aggregate_pubkey) return std::nullopt;
            pubout = *aggregate_pubkey;

            std::unique_ptr<ConstPubkeyProvider> this_agg_provider = std::make_unique<ConstPubkeyProvider>(m_expr_index, aggregate_pubkey.value(), /*xonly=*/false);
            this_agg_provider->GetPubKey(0, dummy, out, read_cache, write_cache);
            out.aggregate_pubkeys.emplace(pubout, pubkeys);
        }

        if (!Assume(pubout.IsValid())) return std::nullopt;
        return pubout;
    }
    bool IsRange() const override { return IsRangedDerivation() || m_ranged_participants; }
    // musig() expressions can only be used in tr() contexts which have 32 byte xonly pubkeys
    size_t GetSize() const override { return 32; }

    std::string ToString(StringType type=StringType::PUBLIC) const override
    {
        std::string out = "musig(";
        for (size_t i = 0; i < m_participants.size(); ++i) {
            const auto& pubkey = m_participants.at(i);
            if (i) out += ",";
            out += pubkey->ToString(type);
        }
        out += ")";
        out += FormatHDKeypath(m_path);
        if (IsRangedDerivation()) {
            out += "/*";
        }
        return out;
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        bool any_privkeys = false;
        out = "musig(";
        for (size_t i = 0; i < m_participants.size(); ++i) {
            const auto& pubkey = m_participants.at(i);
            if (i) out += ",";
            std::string tmp;
            if (pubkey->ToPrivateString(arg, tmp)) {
                any_privkeys = true;
            }
            out += tmp;
        }
        out += ")";
        out += FormatHDKeypath(m_path);
        if (IsRangedDerivation()) {
            out += "/*";
        }
        return any_privkeys;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache = nullptr) const override
    {
        out = "musig(";
        for (size_t i = 0; i < m_participants.size(); ++i) {
            const auto& pubkey = m_participants.at(i);
            if (i) out += ",";
            std::string tmp;
            if (!pubkey->ToNormalizedString(arg, tmp, cache)) {
                return false;
            }
            out += tmp;
        }
        out += ")";
        out += FormatHDKeypath(m_path);
        if (IsRangedDerivation()) {
            out += "/*";
        }
        return true;
    }

    void GetPrivKey(int pos, const SigningProvider& arg, FlatSigningProvider& out) const override
    {
        // Get the private keys for any participants that we have
        // If there is participant derivation, it will be done.
        // If there is not, then the participant privkeys will be included directly
        for (const auto& prov : m_participants) {
            prov->GetPrivKey(pos, arg, out);
        }
    }

    bool HavePrivateKeys(const SigningProvider& arg) const override
    {
        return std::ranges::all_of(m_participants, [&](const auto& prov) { return prov->HavePrivateKeys(arg); });
    }

    // Get RootPubKey and GetRootExtPubKey are used to return the single pubkey underlying the pubkey provider
    // to be presented to the user in gethdkeys. As this is a multisig construction, there is no single underlying
    // pubkey hence nothing should be returned.
    // While the aggregate pubkey could be returned as the root (ext)pubkey, it is not a pubkey that anyone should
    // be using by itself in a descriptor as it is unspendable without knowing its participants.
    std::optional<CPubKey> GetRootPubKey() const override
    {
        return std::nullopt;
    }
    std::optional<CExtPubKey> GetRootExtPubKey() const override
    {
        return std::nullopt;
    }

    std::unique_ptr<PubkeyProvider> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_participants.size());
        for (const std::unique_ptr<PubkeyProvider>& p : m_participants) {
            providers.emplace_back(p->Clone());
        }
        return std::make_unique<MuSigPubkeyProvider>(m_expr_index, std::move(providers), m_path, m_derive);
    }
    bool IsBIP32() const override
    {
        // musig() can only be a BIP 32 key if all participants are bip32 too
        return std::all_of(m_participants.begin(), m_participants.end(), [](const auto& pubkey) { return pubkey->IsBIP32(); });
    }
    size_t GetKeyCount() const override
    {
        return 1 + m_participants.size();
    }
    bool CanSelfExpand() const override
    {
        // Participants must be self expandable for all MuSig expressions to be self expandable; the aggregate pubkey cannot be stored
        // in the descriptor cache, so even aggregate-then-derive still requires the self expansion of participants prior to aggregation.
        for (const auto& key : m_participants) {
            if (!key->CanSelfExpand()) return false;
        }
        return true;
    }
};


void ParseKeyPath(const std::vector<std::span<const char>>& split)
{
    auto parse_elem = [&](std::span<const char> elem) -> std::optional<uint32_t> {
        const auto parsed{ParseKeyPathElement(elem)};
        return parsed->ChildNumber();
    };

    const std::span<const char>& elem = split[0];
    const auto& op_num = parse_elem(elem);
}

} // namespace
