// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BITCOINKERNEL_H
#define BITCOIN_KERNEL_BITCOINKERNEL_H

#ifndef __cplusplus
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
#endif // __cplusplus

#ifndef BITCOINKERNEL_API
#if defined(_WIN32)
#ifdef BITCOINKERNEL_BUILD
#define BITCOINKERNEL_API __declspec(dllexport)
#else
#define BITCOINKERNEL_API
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4) && defined(BITCOINKERNEL_BUILD)
#define BITCOINKERNEL_API __attribute__((visibility("default")))
#else
#define BITCOINKERNEL_API
#endif
#endif

#if !defined(BITCOINKERNEL_GNUC_PREREQ)
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define BITCOINKERNEL_GNUC_PREREQ(_maj, _min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((_maj) << 16) + (_min))
#else
#define BITCOINKERNEL_GNUC_PREREQ(_maj, _min) 0
#endif
#endif

/* Warning attributes */
#if defined(__GNUC__) && BITCOINKERNEL_GNUC_PREREQ(3, 4)
#define BITCOINKERNEL_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#else
#define BITCOINKERNEL_WARN_UNUSED_RESULT
#endif
#if !defined(BITCOINKERNEL_BUILD) && defined(__GNUC__) && BITCOINKERNEL_GNUC_PREREQ(3, 4)
#define BITCOINKERNEL_ARG_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#else
#define BITCOINKERNEL_ARG_NONNULL(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @page remarks Remarks
 *
 * @section context Context
 *
 * The library provides a built-in static constant kernel context. This static
 * context offers only limited functionality. It detects and self-checks the
 * correct sha256 implementation, initializes the random number generator and
 * self-checks the secp256k1 static context. It is used internally for
 * otherwise "context-free" operations. This means that the user is not
 * required to initialize their own context before using the library.
 *
 * @section error Error handling
 *
 * Functions communicate an error through their return types, usually returning
 * a nullptr, or false if an error is encountered. Additionally, verification
 * functions, e.g. for scripts, may communicate more detailed error information
 * through status code out parameters.
 *
 * @section pointer Pointer and argument conventions
 *
 * The user is responsible for de-allocating the memory owned by pointers
 * returned by functions. Typically pointers returned by *_create(...) functions
 * can be de-allocated by corresponding *_destroy(...) functions.
 *
 * A function that takes pointer arguments makes no assumptions on their
 * lifetime. Once the function returns the user can safely de-allocate the
 * passed in arguments.
 *
 * Pointers passed by callbacks are not owned by the user and are only valid
 * for the duration of the callback. They are always marked as `const` and must
 * not be de-allocated by the user.
 *
 * Array lengths follow the pointer argument they describe.
 */

/**
 * Opaque data structure for holding a transaction.
 */
typedef struct kernel_Transaction kernel_Transaction;

/**
 * Opaque data structure for holding a script pubkey.
 */
typedef struct kernel_ScriptPubkey kernel_ScriptPubkey;

/**
 * Opaque data structure for holding a transaction output.
 */
typedef struct kernel_TransactionOutput kernel_TransactionOutput;

/**
 * A collection of status codes that may be issued by the script verify function.
 */
typedef enum {
    kernel_SCRIPT_VERIFY_OK = 0,
    kernel_SCRIPT_VERIFY_ERROR_TX_INPUT_INDEX, //!< The provided input index is out of range of the actual number of inputs of the transaction.
    kernel_SCRIPT_VERIFY_ERROR_INVALID_FLAGS, //!< The provided bitfield for the flags was invalid.
    kernel_SCRIPT_VERIFY_ERROR_INVALID_FLAGS_COMBINATION, //!< The flags very combined in an invalid way.
    kernel_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_REQUIRED, //!< The taproot flag was set, so valid spent_outputs have to be provided.
    kernel_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_MISMATCH, //!< The number of spent outputs does not match the number of inputs of the tx.
} kernel_ScriptVerifyStatus;

/**
 * Script verification flags that may be composed with each other.
 */
typedef enum
{
    kernel_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    kernel_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), //!< evaluate P2SH (BIP16) subscripts
    kernel_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), //!< enforce strict DER (BIP66) compliance
    kernel_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), //!< enforce NULLDUMMY (BIP147)
    kernel_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), //!< enable CHECKLOCKTIMEVERIFY (BIP65)
    kernel_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), //!< enable CHECKSEQUENCEVERIFY (BIP112)
    kernel_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), //!< enable WITNESS (BIP141)

    kernel_SCRIPT_FLAGS_VERIFY_TAPROOT             = (1U << 17), //!< enable TAPROOT (BIPs 341 & 342)
    kernel_SCRIPT_FLAGS_VERIFY_ALL                 = kernel_SCRIPT_FLAGS_VERIFY_P2SH |
                                                     kernel_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                     kernel_SCRIPT_FLAGS_VERIFY_NULLDUMMY |
                                                     kernel_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                     kernel_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY |
                                                     kernel_SCRIPT_FLAGS_VERIFY_WITNESS |
                                                     kernel_SCRIPT_FLAGS_VERIFY_TAPROOT
} kernel_ScriptFlags;

/** @name Transaction
 * Functions for working with transactions.
 */
///@{

/**
 * @brief Create a new transaction from the serialized data.
 *
 * @param[in] raw_transaction     Non-null.
 * @param[in] raw_transaction_len Length of the serialized transaction.
 * @return                        The transaction, or null on error.
 */
BITCOINKERNEL_API kernel_Transaction* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_transaction_create(
    const unsigned char* raw_transaction, size_t raw_transaction_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction.
 */
BITCOINKERNEL_API void kernel_transaction_destroy(kernel_Transaction* transaction);

///@}

/** @name ScriptPubkey
 * Functions for working with script pubkeys.
 */
///@{

/**
 * @brief Create a script pubkey from serialized data.
 * @param[in] script_pubkey     Non-null.
 * @param[in] script_pubkey_len Length of the script pubkey data.
 * @return                      The script pubkey, or null on error.
 */
BITCOINKERNEL_API kernel_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_script_pubkey_create(
    const unsigned char* script_pubkey, size_t script_pubkey_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the script pubkey.
 */
BITCOINKERNEL_API void kernel_script_pubkey_destroy(kernel_ScriptPubkey* script_pubkey);

///@}

/** @name TransactionOutput
 * Functions for working with transaction outputs.
 */
///@{

/**
 * @brief Create a transaction output from a script pubkey and an amount.
 * @param[in] script_pubkey Non-null.
 * @param[in] amount        The amount associated with the script pubkey for this output.
 * @return                  The transaction output.
 */
BITCOINKERNEL_API kernel_TransactionOutput* kernel_transaction_output_create(
    const kernel_ScriptPubkey* script_pubkey,
    int64_t amount
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction output.
 */
BITCOINKERNEL_API void kernel_transaction_output_destroy(kernel_TransactionOutput* transaction_output);

///@}

/** @name Script
 * Functions for working with scripts.
 */
///@{

/**
 * @brief Verify if the input at input_index of tx_to spends the script pubkey
 * under the constraints specified by flags. If the
 * `kernel_SCRIPT_FLAGS_VERIFY_WITNESS` flag is set in the flags bitfield, the
 * amount parameter is used. If the taproot flag is set, the spent outputs
 * parameter is used to validate taproot transactions.
 *
 * @param[in] script_pubkey     Non-null, script pubkey to be spent.
 * @param[in] amount            Amount of the script pubkey's associated output. May be zero if
 *                              the witness flag is not set.
 * @param[in] tx_to             Non-null, transaction spending the script_pubkey.
 * @param[in] spent_outputs     Nullable if the taproot flag is not set. Points to an array of
 *                              outputs spent by the transaction.
 * @param[in] spent_outputs_len Length of the spent_outputs array.
 * @param[in] input_index       Index of the input in tx_to spending the script_pubkey.
 * @param[in] flags             Bitfield of kernel_ScriptFlags controlling validation constraints.
 * @param[out] status           Nullable, will be set to an error code if the operation fails.
 *                              Should be set to kernel_SCRIPT_VERIFY_OK.
 * @return                      True if the script is valid.
 */
BITCOINKERNEL_API bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_verify_script(
    const kernel_ScriptPubkey* script_pubkey,
    int64_t amount,
    const kernel_Transaction* tx_to,
    const kernel_TransactionOutput** spent_outputs, size_t spent_outputs_len,
    unsigned int input_index,
    unsigned int flags,
    kernel_ScriptVerifyStatus* status
) BITCOINKERNEL_ARG_NONNULL(1, 3);

///@}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BITCOIN_KERNEL_BITCOINKERNEL_H
