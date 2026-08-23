// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <addresstype.h>
#include <attributes.h>
#include <consensus/consensus.h>
#include <crypto/hex_base.h>
#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <musig.h>
#include <outputtype.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/keyorigin.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <serialize.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/expected.h>
#include <util/string.h>
#include <util/vector.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
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

/** Base class for all Descriptor implementations. */
class DescriptorImpl : public Descriptor
{
protected:
    //! Public key arguments for this descriptor (size 1 for PK, PKH, WPKH; any size for WSH and Multisig).
    const std::vector<std::unique_ptr<PubkeyProvider>> m_pubkey_args;
    //! The string name of the descriptor function.
    const std::string m_name;
    //! Warnings (not including subdescriptors).
    std::vector<std::string> m_warnings;

    //! The sub-descriptor arguments (empty for everything but SH and WSH).
    //! In doc/descriptors.md this is referred to as SCRIPT expressions sh(SCRIPT)
    //! and wsh(SCRIPT), and distinct from KEY expressions and ADDR expressions.
    //! Subdescriptors can only ever generate a single script.
    const std::vector<std::unique_ptr<DescriptorImpl>> m_subdescriptor_args;

    //! Return a serialization of anything except pubkey and script arguments, to be prepended to those.
    virtual std::string ToStringExtra() const { return ""; }

    /** A helper function to construct the scripts for this descriptor.
     *
     *  This function is invoked once by ExpandHelper.
     *
     *  @param pubkeys The evaluations of the m_pubkey_args field.
     *  @param scripts The evaluations of m_subdescriptor_args (one for each m_subdescriptor_args element).
     *  @param out A FlatSigningProvider to put scripts or public keys in that are necessary to the solver.
     *             The origin info of the provided pubkeys is automatically added.
     *  @return A vector with scriptPubKeys for this descriptor.
     */
    virtual std::vector<CScript> MakeScripts(const std::vector<CPubKey>& pubkeys, std::span<const CScript> scripts, FlatSigningProvider& out) const = 0;

public:
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args() {}
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::unique_ptr<DescriptorImpl> script, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args(Vector(std::move(script))) {}
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::vector<std::unique_ptr<DescriptorImpl>> scripts, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args(std::move(scripts)) {}

    enum class StringType
    {
        PUBLIC,
        PRIVATE,
        NORMALIZED,
        COMPAT, // string calculation that mustn't change over time to stay compatible with previous software versions
    };

    // NOLINTNEXTLINE(misc-no-recursion)
    bool IsSolvable() const override
    {
        for (const auto& arg : m_subdescriptor_args) {
            if (!arg->IsSolvable()) return false;
        }
        return true;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    bool HavePrivateKeys(const SigningProvider& arg) const override
    {
        if (m_pubkey_args.empty() && m_subdescriptor_args.empty()) return false;

        for (const auto& sub: m_subdescriptor_args) {
            if (!sub->HavePrivateKeys(arg)) return false;
        }

        for (const auto& pubkey : m_pubkey_args) {
            if (!pubkey->HavePrivateKeys(arg)) return false;
        }

        return true;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    bool IsRange() const final
    {
        for (const auto& pubkey : m_pubkey_args) {
            if (pubkey->IsRange()) return true;
        }
        for (const auto& arg : m_subdescriptor_args) {
            if (arg->IsRange()) return true;
        }
        return false;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    virtual bool ToStringSubScriptHelper(const SigningProvider* arg, std::string& ret, const StringType type, const DescriptorCache* cache = nullptr) const
    {
        size_t pos = 0;
        bool is_private{type == StringType::PRIVATE};
        // For private string output, track if at least one key has a private key available.
        // Initialize to true for non-private types.
        bool any_success{!is_private};
        for (const auto& scriptarg : m_subdescriptor_args) {
            if (pos++) ret += ",";
            std::string tmp;
            bool subscript_res{scriptarg->ToStringHelper(arg, tmp, type, cache)};
            if (!is_private && !subscript_res) return false;
            any_success = any_success || subscript_res;
            ret += tmp;
        }
        return any_success;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    virtual bool ToStringHelper(const SigningProvider* arg, std::string& out, const StringType type, const DescriptorCache* cache = nullptr) const
    {
        std::string extra = ToStringExtra();
        size_t pos = extra.size() > 0 ? 1 : 0;
        std::string ret = m_name + "(" + extra;
        bool is_private{type == StringType::PRIVATE};
        // For private string output, track if at least one key has a private key available.
        // Initialize to true for non-private types.
        bool any_success{!is_private};

        for (const auto& pubkey : m_pubkey_args) {
            if (pos++) ret += ",";
            std::string tmp;
            switch (type) {
                case StringType::NORMALIZED:
                    if (!pubkey->ToNormalizedString(*arg, tmp, cache)) return false;
                    break;
                case StringType::PRIVATE:
                    any_success = pubkey->ToPrivateString(*arg, tmp) || any_success;
                    break;
                case StringType::PUBLIC:
                    tmp = pubkey->ToString();
                    break;
                case StringType::COMPAT:
                    tmp = pubkey->ToString(PubkeyProvider::StringType::COMPAT);
                    break;
            }
            ret += tmp;
        }
        std::string subscript;
        bool subscript_res{ToStringSubScriptHelper(arg, subscript, type, cache)};
        if (!is_private && !subscript_res) return false;
        any_success = any_success || subscript_res;
        if (pos && subscript.size()) ret += ',';
        out = std::move(ret) + std::move(subscript) + ")";
        return any_success;
    }

    std::string ToString(bool compat_format) const final
    {
        std::string ret;
        ToStringHelper(nullptr, ret, compat_format ? StringType::COMPAT : StringType::PUBLIC);
        return AddChecksum(ret);
    }

    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        bool has_priv_key{ToStringHelper(&arg, out, StringType::PRIVATE)};
        out = AddChecksum(out);
        return has_priv_key;
    }

    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache) const override final
    {
        bool ret = ToStringHelper(&arg, out, StringType::NORMALIZED, cache);
        out = AddChecksum(out);
        return ret;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    bool ExpandHelper(int pos, const SigningProvider& arg, const DescriptorCache* read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache) const
    {
        FlatSigningProvider subprovider;
        std::vector<CPubKey> pubkeys;
        pubkeys.reserve(m_pubkey_args.size());

        // Construct temporary data in `pubkeys`, `subscripts`, and `subprovider` to avoid producing output in case of failure.
        for (const auto& p : m_pubkey_args) {
            std::optional<CPubKey> pubkey = p->GetPubKey(pos, arg, subprovider, read_cache, write_cache);
            if (!pubkey) return false;
            pubkeys.push_back(pubkey.value());
        }
        std::vector<CScript> subscripts;
        for (const auto& subarg : m_subdescriptor_args) {
            std::vector<CScript> outscripts;
            if (!subarg->ExpandHelper(pos, arg, read_cache, outscripts, subprovider, write_cache)) return false;
            assert(outscripts.size() == 1);
            subscripts.emplace_back(std::move(outscripts[0]));
        }
        out.Merge(std::move(subprovider));

        output_scripts = MakeScripts(pubkeys, std::span{subscripts}, out);
        return true;
    }

    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const final
    {
        return ExpandHelper(pos, provider, nullptr, output_scripts, out, write_cache);
    }

    bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const final
    {
        return ExpandHelper(pos, DUMMY_SIGNING_PROVIDER, &read_cache, output_scripts, out, nullptr);
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const final
    {
        for (const auto& p : m_pubkey_args) {
            p->GetPrivKey(pos, provider, out);
        }
        for (const auto& arg : m_subdescriptor_args) {
            arg->ExpandPrivate(pos, provider, out);
        }
    }

    std::optional<OutputType> GetOutputType() const override { return std::nullopt; }

    std::optional<int64_t> ScriptSize() const override { return {}; }

    /** A helper for MaxSatisfactionWeight.
     *
     * @param use_max_sig Whether to assume ECDSA signatures will have a high-r.
     * @return The maximum size of the satisfaction in raw bytes (with no witness meaning).
     */
    virtual std::optional<int64_t> MaxSatSize(bool use_max_sig) const { return {}; }

    std::optional<int64_t> MaxSatisfactionWeight(bool) const override { return {}; }

    std::optional<int64_t> MaxSatisfactionElems() const override { return {}; }

    // NOLINTNEXTLINE(misc-no-recursion)
    void GetPubKeys(std::set<CPubKey>& pubkeys, std::set<CExtPubKey>& ext_pubs) const override
    {
        for (const auto& p : m_pubkey_args) {
            std::optional<CPubKey> pub = p->GetRootPubKey();
            if (pub) pubkeys.insert(*pub);
            std::optional<CExtPubKey> ext_pub = p->GetRootExtPubKey();
            if (ext_pub) ext_pubs.insert(*ext_pub);
        }
        for (const auto& arg : m_subdescriptor_args) {
            arg->GetPubKeys(pubkeys, ext_pubs);
        }
    }

    virtual std::unique_ptr<DescriptorImpl> Clone() const = 0;

    bool HasScripts() const override { return true; }

    // NOLINTNEXTLINE(misc-no-recursion)
    std::vector<std::string> Warnings() const override {
        std::vector<std::string> all = m_warnings;
        for (const auto& sub : m_subdescriptor_args) {
            auto sub_w = sub->Warnings();
            all.insert(all.end(), sub_w.begin(), sub_w.end());
        }
        return all;
    }

    uint32_t GetMaxKeyExpr() const final
    {
        uint32_t max_key_expr{0};
        std::vector<const DescriptorImpl*> todo = {this};
        while (!todo.empty()) {
            const DescriptorImpl* desc = todo.back();
            todo.pop_back();
            for (const auto& p : desc->m_pubkey_args) {
                max_key_expr = std::max(max_key_expr, p->m_expr_index);
            }
            for (const auto& s : desc->m_subdescriptor_args) {
                todo.push_back(s.get());
            }
        }
        return max_key_expr;
    }

    size_t GetKeyCount() const final
    {
        size_t count{0};
        std::vector<const DescriptorImpl*> todo = {this};
        while (!todo.empty()) {
            const DescriptorImpl* desc = todo.back();
            todo.pop_back();
            for (const auto& p : desc->m_pubkey_args) {
                count += p->GetKeyCount();
            }
            for (const auto& s : desc->m_subdescriptor_args) {
                todo.push_back(s.get());
            }
        }
        return count;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    bool CanSelfExpand() const override
    {
        for (const auto& key : m_pubkey_args) {
            if (!key->CanSelfExpand()) return false;
        }
        for (const auto& sub : m_subdescriptor_args) {
            if (!sub->CanSelfExpand()) return false;
        }
        return true;
    }
};

/** A parsed addr(A) descriptor. */
class AddressDescriptor final : public DescriptorImpl
{
    const CTxDestination m_destination;
protected:
    std::string ToStringExtra() const override { return EncodeDestination(m_destination); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript>, FlatSigningProvider&) const override { return Vector(GetScriptForDestination(m_destination)); }
public:
    AddressDescriptor(CTxDestination destination) : DescriptorImpl({}, "addr"), m_destination(std::move(destination)) {}
    bool IsSolvable() const final { return false; }

    std::optional<OutputType> GetOutputType() const override
    {
        return OutputTypeFromDestination(m_destination);
    }
    bool IsSingleType() const final { return true; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const final { return false; }

    std::optional<int64_t> ScriptSize() const override { return GetScriptForDestination(m_destination).size(); }
    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<AddressDescriptor>(m_destination);
    }
};

/** A parsed raw(H) descriptor. */
class RawDescriptor final : public DescriptorImpl
{
    const CScript m_script;
protected:
    std::string ToStringExtra() const override { return HexStr(m_script); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript>, FlatSigningProvider&) const override { return Vector(m_script); }
public:
    RawDescriptor(CScript script) : DescriptorImpl({}, "raw"), m_script(std::move(script)) {}
    bool IsSolvable() const final { return false; }

    std::optional<OutputType> GetOutputType() const override
    {
        CTxDestination dest;
        ExtractDestination(m_script, dest);
        return OutputTypeFromDestination(dest);
    }
    bool IsSingleType() const final { return true; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const final { return false; }

    std::optional<int64_t> ScriptSize() const override { return m_script.size(); }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<RawDescriptor>(m_script);
    }
};

/** A parsed pk(P) descriptor. */
class PKDescriptor final : public DescriptorImpl
{
private:
    const bool m_xonly;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override
    {
        if (m_xonly) {
            CScript script = CScript() << ToByteVector(XOnlyPubKey(keys[0])) << OP_CHECKSIG;
            return Vector(std::move(script));
        } else {
            return Vector(GetScriptForRawPubKey(keys[0]));
        }
    }
public:
    PKDescriptor(std::unique_ptr<PubkeyProvider> prov, bool xonly = false) : DescriptorImpl(Vector(std::move(prov)), "pk"), m_xonly(xonly) {}
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override {
        return 1 + (m_xonly ? 32 : m_pubkey_args[0]->GetSize()) + 1;
    }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto ecdsa_sig_size = use_max_sig ? 72 : 71;
        return 1 + (m_xonly ? 65 : ecdsa_sig_size);
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return *MaxSatSize(use_max_sig) * WITNESS_SCALE_FACTOR;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 1; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<PKDescriptor>(m_pubkey_args.at(0)->Clone(), m_xonly);
    }
};

/** A parsed pkh(P) descriptor. */
class PKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override
    {
        CKeyID id = keys[0].GetID();
        return Vector(GetScriptForDestination(PKHash(id)));
    }
public:
    PKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "pkh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::LEGACY; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 1 + 20 + 1 + 1; }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto sig_size = use_max_sig ? 72 : 71;
        return 1 + sig_size + 1 + m_pubkey_args[0]->GetSize();
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return *MaxSatSize(use_max_sig) * WITNESS_SCALE_FACTOR;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 2; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<PKHDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed wpkh(P) descriptor. */
class WPKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override
    {
        CKeyID id = keys[0].GetID();
        return Vector(GetScriptForDestination(WitnessV0KeyHash(id)));
    }
public:
    WPKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "wpkh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 20; }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto sig_size = use_max_sig ? 72 : 71;
        return (1 + sig_size + 1 + 33);
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return MaxSatSize(use_max_sig);
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 2; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<WPKHDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed combo(P) descriptor. */
class ComboDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider& out) const override
    {
        std::vector<CScript> ret;
        CKeyID id = keys[0].GetID();
        ret.emplace_back(GetScriptForRawPubKey(keys[0])); // P2PK
        ret.emplace_back(GetScriptForDestination(PKHash(id))); // P2PKH
        if (keys[0].IsCompressed()) {
            CScript p2wpkh = GetScriptForDestination(WitnessV0KeyHash(id));
            out.scripts.emplace(CScriptID(p2wpkh), p2wpkh);
            ret.emplace_back(p2wpkh);
            ret.emplace_back(GetScriptForDestination(ScriptHash(p2wpkh))); // P2SH-P2WPKH
        }
        return ret;
    }
public:
    ComboDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "combo") {}
    bool IsSingleType() const final { return false; }
    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<ComboDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed multi(...) or sortedmulti(...) descriptor */
class MultisigDescriptor final : public DescriptorImpl
{
    const int m_threshold;
    const bool m_sorted;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override {
        if (m_sorted) {
            std::vector<CPubKey> sorted_keys(keys);
            std::sort(sorted_keys.begin(), sorted_keys.end());
            return Vector(GetScriptForMultisig(m_threshold, sorted_keys));
        }
        return Vector(GetScriptForMultisig(m_threshold, keys));
    }
public:
    MultisigDescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers, bool sorted = false) : DescriptorImpl(std::move(providers), sorted ? "sortedmulti" : "multi"), m_threshold(threshold), m_sorted(sorted) {}
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override {
        const auto n_keys = m_pubkey_args.size();
        auto op = [](int64_t acc, const std::unique_ptr<PubkeyProvider>& pk) { return acc + 1 + pk->GetSize();};
        const auto pubkeys_size{std::accumulate(m_pubkey_args.begin(), m_pubkey_args.end(), int64_t{0}, op)};
        return 1 + BuildScript(n_keys).size() + BuildScript(m_threshold).size() + pubkeys_size;
    }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        const auto sig_size = use_max_sig ? 72 : 71;
        return (1 + (1 + sig_size) * m_threshold);
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return *MaxSatSize(use_max_sig) * WITNESS_SCALE_FACTOR;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return 1 + m_threshold; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_pubkey_args.size());
        std::transform(m_pubkey_args.begin(), m_pubkey_args.end(), std::back_inserter(providers), [](const std::unique_ptr<PubkeyProvider>& p) { return p->Clone(); });
        return std::make_unique<MultisigDescriptor>(m_threshold, std::move(providers), m_sorted);
    }
};

/** A parsed (sorted)multi_a(...) descriptor. Always uses x-only pubkeys. */
class MultiADescriptor final : public DescriptorImpl
{
    const int m_threshold;
    const bool m_sorted;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript>, FlatSigningProvider&) const override {
        CScript ret;
        std::vector<XOnlyPubKey> xkeys;
        xkeys.reserve(keys.size());
        for (const auto& key : keys) xkeys.emplace_back(key);
        if (m_sorted) std::sort(xkeys.begin(), xkeys.end());
        ret << ToByteVector(xkeys[0]) << OP_CHECKSIG;
        for (size_t i = 1; i < keys.size(); ++i) {
            ret << ToByteVector(xkeys[i]) << OP_CHECKSIGADD;
        }
        ret << m_threshold << OP_NUMEQUAL;
        return Vector(std::move(ret));
    }
public:
    MultiADescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers, bool sorted = false) : DescriptorImpl(std::move(providers), sorted ? "sortedmulti_a" : "multi_a"), m_threshold(threshold), m_sorted(sorted) {}
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override {
        const auto n_keys = m_pubkey_args.size();
        return (1 + 32 + 1) * n_keys + BuildScript(m_threshold).size() + 1;
    }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        return (1 + 65) * m_threshold + (m_pubkey_args.size() - m_threshold);
    }

    std::optional<int64_t> MaxSatisfactionElems() const override { return m_pubkey_args.size(); }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_pubkey_args.size());
        for (const auto& arg : m_pubkey_args) {
            providers.push_back(arg->Clone());
        }
        return std::make_unique<MultiADescriptor>(m_threshold, std::move(providers), m_sorted);
    }
};

/** A parsed sh(...) descriptor. */
class SHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        auto ret = Vector(GetScriptForDestination(ScriptHash(scripts[0])));
        if (ret.size()) out.scripts.emplace(CScriptID(scripts[0]), scripts[0]);
        return ret;
    }

    bool IsSegwit() const { return m_subdescriptor_args[0]->GetOutputType() == OutputType::BECH32; }

public:
    SHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "sh") {}

    std::optional<OutputType> GetOutputType() const override
    {
        assert(m_subdescriptor_args.size() == 1);
        if (IsSegwit()) return OutputType::P2SH_SEGWIT;
        return OutputType::LEGACY;
    }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 20 + 1; }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        if (const auto sat_size = m_subdescriptor_args[0]->MaxSatSize(use_max_sig)) {
            if (const auto subscript_size = m_subdescriptor_args[0]->ScriptSize()) {
                // The subscript is never witness data.
                const auto subscript_weight = (1 + *subscript_size) * WITNESS_SCALE_FACTOR;
                // The weight depends on whether the inner descriptor is satisfied using the witness stack.
                if (IsSegwit()) return subscript_weight + *sat_size;
                return subscript_weight + *sat_size * WITNESS_SCALE_FACTOR;
            }
        }
        return {};
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        if (const auto sub_elems = m_subdescriptor_args[0]->MaxSatisfactionElems()) return 1 + *sub_elems;
        return {};
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<SHDescriptor>(m_subdescriptor_args.at(0)->Clone());
    }
};

/** A parsed wsh(...) descriptor. */
class WSHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        auto ret = Vector(GetScriptForDestination(WitnessV0ScriptHash(scripts[0])));
        if (ret.size()) out.scripts.emplace(CScriptID(scripts[0]), scripts[0]);
        return ret;
    }
public:
    WSHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "wsh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 32; }

    std::optional<int64_t> MaxSatSize(bool use_max_sig) const override {
        if (const auto sat_size = m_subdescriptor_args[0]->MaxSatSize(use_max_sig)) {
            if (const auto subscript_size = m_subdescriptor_args[0]->ScriptSize()) {
                return GetSizeOfCompactSize(*subscript_size) + *subscript_size + *sat_size;
            }
        }
        return {};
    }

    std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const override {
        return MaxSatSize(use_max_sig);
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        if (const auto sub_elems = m_subdescriptor_args[0]->MaxSatisfactionElems()) return 1 + *sub_elems;
        return {};
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<WSHDescriptor>(m_subdescriptor_args.at(0)->Clone());
    }
};

/** A parsed tr(...) descriptor. */
class TRDescriptor final : public DescriptorImpl
{
    std::vector<int> m_depths;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        TaprootBuilder builder;
        assert(m_depths.size() == scripts.size());
        for (size_t pos = 0; pos < m_depths.size(); ++pos) {
            builder.Add(m_depths[pos], scripts[pos], TAPROOT_LEAF_TAPSCRIPT);
        }
        if (!builder.IsComplete()) return {};
        assert(keys.size() == 1);
        XOnlyPubKey xpk(keys[0]);
        if (!xpk.IsFullyValid()) return {};
        builder.Finalize(xpk);
        WitnessV1Taproot output = builder.GetOutput();
        out.tr_trees[output] = builder;
        return Vector(GetScriptForDestination(output));
    }
    bool ToStringSubScriptHelper(const SigningProvider* arg, std::string& ret, const StringType type, const DescriptorCache* cache = nullptr) const override
    {
        if (m_depths.empty()) {
            // If there are no sub-descriptors and a PRIVATE string
            // is requested, return `false` to indicate that the presence
            // of a private key depends solely on the internal key (which is checked
            // in the caller), not on any sub-descriptor. This ensures correct behavior for
            // descriptors like tr(internal_key) when checking for private keys.
            return type != StringType::PRIVATE;
        }
        std::vector<bool> path;
        bool is_private{type == StringType::PRIVATE};
        // For private string output, track if at least one key has a private key available.
        // Initialize to true for non-private types.
        bool any_success{!is_private};

        for (size_t pos = 0; pos < m_depths.size(); ++pos) {
            if (pos) ret += ',';
            while ((int)path.size() <= m_depths[pos]) {
                if (path.size()) ret += '{';
                path.push_back(false);
            }
            std::string tmp;
            bool subscript_res{m_subdescriptor_args[pos]->ToStringHelper(arg, tmp, type, cache)};
            if (!is_private && !subscript_res) return false;
            any_success = any_success || subscript_res;
            ret += tmp;
            while (!path.empty() && path.back()) {
                if (path.size() > 1) ret += '}';
                path.pop_back();
            }
            if (!path.empty()) path.back() = true;
        }
        return any_success;
    }
public:
    TRDescriptor(std::unique_ptr<PubkeyProvider> internal_key, std::vector<std::unique_ptr<DescriptorImpl>> descs, std::vector<int> depths) :
        DescriptorImpl(Vector(std::move(internal_key)), std::move(descs), "tr"), m_depths(std::move(depths))
    {
        assert(m_subdescriptor_args.size() == m_depths.size());
    }
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32M; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 32; }

    std::optional<int64_t> MaxSatisfactionWeight(bool) const override {
        // FIXME: We assume keypath spend, which can lead to very large underestimations.
        return 1 + 65;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        // FIXME: See above, we assume keypath spend.
        return 1;
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<DescriptorImpl>> subdescs;
        subdescs.reserve(m_subdescriptor_args.size());
        std::transform(m_subdescriptor_args.begin(), m_subdescriptor_args.end(), std::back_inserter(subdescs), [](const std::unique_ptr<DescriptorImpl>& d) { return d->Clone(); });
        return std::make_unique<TRDescriptor>(m_pubkey_args.at(0)->Clone(), std::move(subdescs), m_depths);
    }
};

/* We instantiate Miniscript here with a simple integer as key type.
 * The value of these key integers are an index in the
 * DescriptorImpl::m_pubkey_args vector.
 */

/**
 * The context for converting a Miniscript descriptor into a Script.
 */
class ScriptMaker {
    //! Keys contained in the Miniscript (the evaluation of DescriptorImpl::m_pubkey_args).
    const std::vector<CPubKey>& m_keys;
    //! The script context we're operating within (Tapscript or P2WSH).
    const miniscript::MiniscriptContext m_script_ctx;

    //! Get the ripemd160(sha256()) hash of this key.
    //! Any key that is valid in a descriptor serializes as 32 bytes within a Tapscript context. So we
    //! must not hash the sign-bit byte in this case.
    uint160 GetHash160(uint32_t key) const {
        if (miniscript::IsTapscript(m_script_ctx)) {
            return Hash160(XOnlyPubKey{m_keys[key]});
        }
        return m_keys[key].GetID();
    }

public:
    ScriptMaker(const std::vector<CPubKey>& keys LIFETIMEBOUND, const miniscript::MiniscriptContext script_ctx) : m_keys(keys), m_script_ctx{script_ctx} {}

    std::vector<unsigned char> ToPKBytes(uint32_t key) const {
        // In Tapscript keys always serialize as x-only, whether an x-only key was used in the descriptor or not.
        if (!miniscript::IsTapscript(m_script_ctx)) {
            return {m_keys[key].begin(), m_keys[key].end()};
        }
        const XOnlyPubKey xonly_pubkey{m_keys[key]};
        return {xonly_pubkey.begin(), xonly_pubkey.end()};
    }

    std::vector<unsigned char> ToPKHBytes(uint32_t key) const {
        auto id = GetHash160(key);
        return {id.begin(), id.end()};
    }
};

/**
 * The context for converting a Miniscript descriptor to its textual form.
 */
class StringMaker {
    //! To convert private keys for private descriptors.
    const SigningProvider* m_arg;
    //! Keys contained in the Miniscript (a reference to DescriptorImpl::m_pubkey_args).
    const std::vector<std::unique_ptr<PubkeyProvider>>& m_pubkeys;
    //! StringType to serialize keys
    const DescriptorImpl::StringType m_type;
    const DescriptorCache* m_cache;

public:
    StringMaker(const SigningProvider* arg LIFETIMEBOUND,
                const std::vector<std::unique_ptr<PubkeyProvider>>& pubkeys LIFETIMEBOUND,
                DescriptorImpl::StringType type,
                const DescriptorCache* cache LIFETIMEBOUND)
        : m_arg(arg), m_pubkeys(pubkeys), m_type(type), m_cache(cache) {}

    std::optional<std::string> ToString(uint32_t key, bool& has_priv_key) const
    {
        std::string ret;
        has_priv_key = false;
        switch (m_type) {
        case DescriptorImpl::StringType::PUBLIC:
            ret = m_pubkeys[key]->ToString();
            break;
        case DescriptorImpl::StringType::PRIVATE:
            has_priv_key = m_pubkeys[key]->ToPrivateString(*m_arg, ret);
            break;
        case DescriptorImpl::StringType::NORMALIZED:
            if (!m_pubkeys[key]->ToNormalizedString(*m_arg, ret, m_cache)) return {};
            break;
        case DescriptorImpl::StringType::COMPAT:
            ret = m_pubkeys[key]->ToString(PubkeyProvider::StringType::COMPAT);
            break;
        }
        return ret;
    }
};

class MiniscriptDescriptor final : public DescriptorImpl
{
private:
    miniscript::Node<uint32_t> m_node;

protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts,
                                     FlatSigningProvider& provider) const override
    {
        const auto script_ctx{m_node.GetMsCtx()};
        for (const auto& key : keys) {
            if (miniscript::IsTapscript(script_ctx)) {
                provider.pubkeys.emplace(Hash160(XOnlyPubKey{key}), key);
            } else {
                provider.pubkeys.emplace(key.GetID(), key);
            }
        }
        return Vector(m_node.ToScript(ScriptMaker(keys, script_ctx)));
    }

public:
    MiniscriptDescriptor(std::vector<std::unique_ptr<PubkeyProvider>> providers, miniscript::Node<uint32_t>&& node)
        : DescriptorImpl(std::move(providers), "?"), m_node(std::move(node))
    {
        // Traverse miniscript tree for unsafe use of older()
        miniscript::ForEachNode(m_node, [&](const miniscript::Node<uint32_t>& node) {
            if (node.Fragment() == miniscript::Fragment::OLDER) {
                const uint32_t raw = node.K();
                const uint32_t value_part = raw & ~CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
                if (value_part > CTxIn::SEQUENCE_LOCKTIME_MASK) {
                    const bool is_time_based = (raw & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) != 0;
                    if (is_time_based) {
                        m_warnings.push_back(strprintf("time-based relative locktime: older(%u) > (65535 * 512) seconds is unsafe", raw));
                    } else {
                        m_warnings.push_back(strprintf("height-based relative locktime: older(%u) > 65535 blocks is unsafe", raw));
                    }
                }
            }
        });
    }

    bool ToStringHelper(const SigningProvider* arg, std::string& out, const StringType type,
                        const DescriptorCache* cache = nullptr) const override
    {
        bool has_priv_key{false};
        auto res = m_node.ToString(StringMaker(arg, m_pubkey_args, type, cache), has_priv_key);
        if (res) out = *res;
        if (type == StringType::PRIVATE) {
            Assume(res.has_value());
            return has_priv_key;
        } else {
            return res.has_value();
        }
    }

    bool IsSolvable() const override { return true; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return m_node.ScriptSize(); }

    std::optional<int64_t> MaxSatSize(bool) const override
    {
        // For Miniscript we always assume high-R ECDSA signatures.
        return m_node.GetWitnessSize();
    }

    std::optional<int64_t> MaxSatisfactionElems() const override
    {
        return m_node.GetStackSize();
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        providers.reserve(m_pubkey_args.size());
        for (const auto& arg : m_pubkey_args) {
            providers.push_back(arg->Clone());
        }
        return std::make_unique<MiniscriptDescriptor>(std::move(providers), m_node.Clone());
    }
};

/** A parsed rawtr(...) descriptor. */
class RawTRDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        assert(keys.size() == 1);
        XOnlyPubKey xpk(keys[0]);
        if (!xpk.IsFullyValid()) return {};
        WitnessV1Taproot output{xpk};
        return Vector(GetScriptForDestination(output));
    }
public:
    RawTRDescriptor(std::unique_ptr<PubkeyProvider> output_key) : DescriptorImpl(Vector(std::move(output_key)), "rawtr") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32M; }
    bool IsSingleType() const final { return true; }

    std::optional<int64_t> ScriptSize() const override { return 1 + 1 + 32; }

    std::optional<int64_t> MaxSatisfactionWeight(bool) const override {
        // We can't know whether there is a script path, so assume key path spend.
        return 1 + 65;
    }

    std::optional<int64_t> MaxSatisfactionElems() const override {
        // See above, we assume keypath spend.
        return 1;
    }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<RawTRDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};

/** A parsed unused(KEY) descriptor */
class UnusedDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, std::span<const CScript> scripts, FlatSigningProvider& out) const override { return {}; }
public:
    UnusedDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "unused") {}
    bool IsSingleType() const final { return true; }
    bool HasScripts() const override { return false; }

    std::unique_ptr<DescriptorImpl> Clone() const override
    {
        return std::make_unique<UnusedDescriptor>(m_pubkey_args.at(0)->Clone());
    }
};


////////////////////////////////////////////////////////////////////////////
// Parser                                                                 //
////////////////////////////////////////////////////////////////////////////

enum class ParseScriptContext {
    TOP,     //!< Top-level context (script goes directly in scriptPubKey)
    P2SH,    //!< Inside sh() (script becomes P2SH redeemScript)
    P2WPKH,  //!< Inside wpkh() (no script, pubkey only)
    P2WSH,   //!< Inside wsh() (script becomes v0 witness script)
    P2TR,    //!< Inside tr() (either internal key, or BIP342 script leaf)
    MUSIG,   //!< Inside musig() (implies P2TR, cannot have nested musig())
};

bool ParseKeyPath(const std::vector<std::span<const char>>& split, std::vector<KeyPath>& out, bool& apostrophe, std::string& error, bool allow_multipath, bool& has_hardened)
{
    auto parse_elem = [&](std::span<const char> elem) -> std::optional<uint32_t> {
        const auto parsed{ParseKeyPathElement(elem)};
        return parsed->ChildNumber();
    };

    const std::span<const char>& elem = split[0];
    const auto& op_num = parse_elem(elem);

    return true;
}

} // namespace
