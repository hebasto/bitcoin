// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_DESCRIPTOR_H
#define BITCOIN_SCRIPT_DESCRIPTOR_H

#include <outputtype.h>
#include <pubkey.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class CScript;
class SigningProvider;
struct FlatSigningProvider;

using ExtPubKeyMap = std::unordered_map<uint32_t, CExtPubKey>;

/** Cache for single descriptor's derived extended pubkeys */
class DescriptorCache {
};

/** \brief Interface for parsed descriptor objects.
 *
 * Descriptors are strings that describe a set of scriptPubKeys, together with
 * all information necessary to solve them. By combining all information into
 * one, they avoid the need to separately import keys and scripts.
 *
 * Descriptors may be ranged, which occurs when the public keys inside are
 * specified in the form of HD chains (xpubs).
 *
 * Descriptors always represent public information - public keys and scripts -
 * but in cases where private keys need to be conveyed along with a descriptor,
 * they can be included inside by changing public keys to private keys (WIF
 * format), and changing xpubs by xprvs.
 *
 * Reference documentation about the descriptor language can be found in
 * doc/descriptors.md.
 */
struct Descriptor {
    virtual ~Descriptor() = default;

    /** Whether the expansion of this descriptor depends on the position. */
    virtual bool IsRange() const = 0;

    /** Whether this descriptor has all information about signing ignoring lack of private keys.
     *  This is true for all descriptors except ones that use `raw` or `addr` constructions. */
    virtual bool IsSolvable() const = 0;

    /** Convert the descriptor back to a string, undoing parsing. */
    virtual std::string ToString(bool compat_format=false) const = 0;

    /** Whether this descriptor will return at most one scriptPubKey or multiple (aka is or is not combo) */
    virtual bool IsSingleType() const = 0;

    /** Whether the given provider has all private keys required by this descriptor.
     * @return `false` if the descriptor doesn't have any keys or subdescriptors,
     *         or if the provider does not have all private keys required by
     *         the descriptor.
     */
    virtual bool HavePrivateKeys(const SigningProvider& provider) const = 0;

    /** Convert the descriptor to a private string. This uses public keys if the relevant private keys are not in the SigningProvider.
     *  If none of the relevant private keys are available, the output string in the "out" parameter will not contain any private key information,
     *  and this function will return "false".
     *  @param[in] provider The SigningProvider to query for private keys.
     *  @param[out] out The resulting descriptor string, containing private keys if available.
     *  @returns true if at least one private key available.
     */
    virtual bool ToPrivateString(const SigningProvider& provider, std::string& out) const = 0;

    /** Convert the descriptor to a normalized string. Normalized descriptors have the xpub at the last hardened step. This fails if the provided provider does not have the private keys to derive that xpub. */
    virtual bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const = 0;

    /** Whether the descriptor can be used to produce its address(es) without needing a cache or private keys. */
    virtual bool CanSelfExpand() const = 0;

    /** Expand a descriptor at a specified position.
     *
     * @param[in] pos The position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * @param[in] provider The provider to query for private keys in case of hardened derivation.
     * @param[out] output_scripts The expanded scriptPubKeys.
     * @param[out] out Scripts and public keys necessary for solving the expanded scriptPubKeys (may be equal to `provider`).
     * @param[out] write_cache Cache data necessary to evaluate the descriptor at this point without access to private keys.
     */
    virtual bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const = 0;

    /** Expand a descriptor at a specified position using cached expansion data.
     *
     * @param[in] pos The position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * @param[in] read_cache Cached expansion data.
     * @param[out] output_scripts The expanded scriptPubKeys.
     * @param[out] out Scripts and public keys necessary for solving the expanded scriptPubKeys (may be equal to `provider`).
     */
    virtual bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const = 0;

    /** Expand the private key for a descriptor at a specified position, if possible.
     *
     * @param[in] pos The position at which to expand the descriptor. If IsRange() is false, this is ignored.
     * @param[in] provider The provider to query for the private keys.
     * @param[out] out Any private keys available for the specified `pos`.
     */
    virtual void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const = 0;

    /** @return The OutputType of the scriptPubKey(s) produced by this descriptor. Or nullopt if indeterminate (multiple or none) */
    virtual std::optional<OutputType> GetOutputType() const = 0;

    /** Get the size of the scriptPubKey for this descriptor. */
    virtual std::optional<int64_t> ScriptSize() const = 0;

    /** Get the maximum size of a satisfaction for this descriptor, in weight units.
     *
     * @param use_max_sig Whether to assume ECDSA signatures will have a high-r.
     */
    virtual std::optional<int64_t> MaxSatisfactionWeight(bool use_max_sig) const = 0;

    /** Get the maximum size number of stack elements for satisfying this descriptor. */
    virtual std::optional<int64_t> MaxSatisfactionElems() const = 0;

    /** Return all (extended) public keys for this descriptor, including any from subdescriptors.
     *
     * @param[out] pubkeys Any public keys
     * @param[out] ext_pubs Any extended public keys
     */
    virtual void GetPubKeys(std::set<CPubKey>& pubkeys, std::set<CExtPubKey>& ext_pubs) const = 0;

    /** Whether this descriptor produces any scripts with the Expand functions */
    virtual bool HasScripts() const = 0;

    /** Semantic/safety warnings (includes subdescriptors). */
    virtual std::vector<std::string> Warnings() const = 0;

    /** Get the maximum key expression index. Used only for tests */
    virtual uint32_t GetMaxKeyExpr() const = 0;

    /** Get the number of key expressions in this descriptor. Used only for tests */
    virtual size_t GetKeyCount() const = 0;
};

#endif // BITCOIN_SCRIPT_DESCRIPTOR_H
