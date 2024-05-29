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
 * The user should create their own context for passing it to state-rich validation
 * functions and holding callbacks for kernel events.
 *
 * @section error Error handling
 *
 * Functions communicate an error through their return types, usually returning
 * a nullptr, or false if an error is encountered. Additionally, verification
 * functions, e.g. for scripts, may communicate more detailed error information
 * through status code out parameters.
 *
 * The kernel notifications issue callbacks for errors. These are usually
 * indicative of a system error. If such an error is issued, it is recommended
 * to halt and tear down the existing kernel objects. Remediating the error may
 * require system intervention by the user.
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
 * Opaque data structure for holding a logging connection.
 *
 * The logging connection can be used to manually stop logging.
 *
 * Messages that were logged before a connection is created are buffered in a
 * 1MB buffer. Logging can alternatively be permanently disabled by calling
 * kernel_disable_logging(). Functions changing the logging settings are global
 * (and not thread safe) and change the settings for all existing
 * kernel_LoggingConnection instances.
 */
typedef struct kernel_LoggingConnection kernel_LoggingConnection;

/**
 * Opaque data structure for holding the chain parameters.
 *
 * These are eventually placed into a kernel context through the kernel context
 * options. The parameters describe the properties of a chain, and may be
 * instantiated for either mainnet, testnet, signet, or regtest.
 */
typedef struct kernel_ChainParameters kernel_ChainParameters;

/**
 * Opaque data structure for holding options for creating a new kernel context.
 *
 * Once a kernel context has been created from these options, they may be
 * destroyed. The options hold the notification callbacks as well as the
 * selected chain type until they are passed to the context. If no options are
 * configured, the context will be instantiated with no callbacks and for
 * mainnet. Their content and scope can be expanded over time.
 */
typedef struct kernel_ContextOptions kernel_ContextOptions;

/**
 * Opaque data structure for holding a kernel context.
 *
 * The kernel context is used to initialize internal state and hold the chain
 * parameters and callbacks for handling error and validation events. Once other
 * validation objects are instantiated from it, the context needs to be kept in
 * memory for the duration of their lifetimes.
 *
 * A constructed context can be safely used from multiple threads.
 */
typedef struct kernel_Context kernel_Context;

/**
 * Opaque data structure for holding a block index pointer.
 *
 * This is a pointer to an element in the block index currently in memory of the
 * chainstate manager. It is valid for the lifetime of the chainstate manager it
 * was retrieved from.
 */
typedef struct kernel_BlockIndex kernel_BlockIndex;

/**
 * Opaque data structure for holding options for creating a new chainstate
 * manager.
 *
 * The chainstate manager options are used to set some parameters for the
 * chainstate manager. For now it just holds default options.
 */
typedef struct kernel_ChainstateManagerOptions kernel_ChainstateManagerOptions;

/**
 * Opaque data structure for holding a chainstate manager.
 *
 * The chainstate manager is the central object for doing validation tasks as
 * well as retrieving data from the chain. Internally it is a complex data
 * structure with diverse functionality.
 *
 * The chainstate manager is only valid for as long as the context with which it
 * was created remains in memory.
 *
 * Its functionality will be more and more exposed in the future.
 */
typedef struct kernel_ChainstateManager kernel_ChainstateManager;

/** Current sync state passed to tip changed callbacks. */
typedef enum {
    kernel_INIT_REINDEX,
    kernel_INIT_DOWNLOAD,
    kernel_POST_INIT
} kernel_SynchronizationState;

/** Possible warning types issued by validation. */
typedef enum {
    kernel_UNKNOWN_NEW_RULES_ACTIVATED,
    kernel_LARGE_WORK_INVALID_CHAIN
} kernel_Warning;

/** Callback function types */

/**
 * Function signature for the global logging callback. All bitcoin kernel
 * internal logs will pass through this callback.
 */
typedef void (*kernel_LogCallback)(void* user_data, const char* message, size_t message_len);

/**
 * Function signatures for the kernel notifications.
 */
typedef void (*kernel_NotifyBlockTip)(void* user_data, kernel_SynchronizationState state, const kernel_BlockIndex* index);
typedef void (*kernel_NotifyHeaderTip)(void* user_data, kernel_SynchronizationState state, int64_t height, int64_t timestamp, bool presync);
typedef void (*kernel_NotifyProgress)(void* user_data, const char* title, size_t title_len, int progress_percent, bool resume_possible);
typedef void (*kernel_NotifyWarningSet)(void* user_data, kernel_Warning warning, const char* message, size_t message_len);
typedef void (*kernel_NotifyWarningUnset)(void* user_data, kernel_Warning warning);
typedef void (*kernel_NotifyFlushError)(void* user_data, const char* message, size_t message_len);
typedef void (*kernel_NotifyFatalError)(void* user_data, const char* message, size_t message_len);

/**
 * A struct for holding the kernel notification callbacks. The user data
 * pointer may be used to point to user-defined structures to make processing
 * the notifications easier. Note that this makes it the user's responsibility
 * to ensure that the user_data outlives the kernel objects. Notifications can
 * occur even as kernel objects are deleted, so care has to be taken to ensure
 * safe unwinding.
 */
typedef struct {
    const void* user_data;                   //!< Holds a user-defined opaque structure that is passed to the notification callbacks.
    kernel_NotifyBlockTip block_tip;         //!< The chain's tip was updated to the provided block index.
    kernel_NotifyHeaderTip header_tip;       //!< A new best block header was added.
    kernel_NotifyProgress progress;          //!< Reports on current block synchronization progress.
    kernel_NotifyWarningSet warning_set;     //!< A warning issued by the kernel library during validation.
    kernel_NotifyWarningUnset warning_unset; //!< A previous condition leading to the issuance of a warning is no longer given.
    kernel_NotifyFlushError flush_error;     //!< An error encountered when flushing data to disk.
    kernel_NotifyFatalError fatal_error;     //!< A un-recoverable system error encountered by the library.
} kernel_NotificationInterfaceCallbacks;

/**
 * A collection of logging categories that may be encountered by kernel code.
 */
typedef enum {
    kernel_LOG_ALL = 0,
    kernel_LOG_BENCH,
    kernel_LOG_BLOCKSTORAGE,
    kernel_LOG_COINDB,
    kernel_LOG_LEVELDB,
    kernel_LOG_MEMPOOL,
    kernel_LOG_PRUNE,
    kernel_LOG_RAND,
    kernel_LOG_REINDEX,
    kernel_LOG_VALIDATION,
    kernel_LOG_KERNEL,
} kernel_LogCategory;

/**
 * The level at which logs should be produced.
 */
typedef enum {
    kernel_LOG_TRACE = 0,
    kernel_LOG_DEBUG,
    kernel_LOG_INFO,
} kernel_LogLevel;

/**
 * Options controlling the format of log messages.
 */
typedef struct {
    bool log_timestamps;               //!< Prepend a timestamp to log messages.
    bool log_time_micros;              //!< Log timestamps in microsecond precision.
    bool log_threadnames;              //!< Prepend the name of the thread to log messages.
    bool log_sourcelocations;          //!< Prepend the source location to log messages.
    bool always_print_category_levels; //!< Prepend the log category and level to log messages.
} kernel_LoggingOptions;

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

/**
 * Chain type used for creating chain params.
 */
typedef enum {
    kernel_CHAIN_TYPE_MAINNET = 0,
    kernel_CHAIN_TYPE_TESTNET,
    kernel_CHAIN_TYPE_TESTNET_4,
    kernel_CHAIN_TYPE_SIGNET,
    kernel_CHAIN_TYPE_REGTEST,
} kernel_ChainType;

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

/** @name Logging
 * Logging-related functions.
 */
///@{

/**
 * @brief This disables the global internal logger. No log messages will be
 * buffered internally anymore once this is called and the buffer is cleared.
 * This function should only be called once and is not thread or re-entry safe.
 * Log messages will be buffered until this function is called, or a logging
 * connection is created.
 */
BITCOINKERNEL_API void kernel_disable_logging();

/**
 * @brief Set the log level of the global internal logger. This does not
 * enable the selected categories. Use `kernel_enable_log_category` to start
 * logging from a specific, or all categories. This function is not thread
 * safe. Mutiple calls from different threads are allowed but must be
 * synchronized. This changes a global setting and will override settings for
 * all existing @ref kernel_LoggingConnection instances.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all messages at the specified level
 *                     will be logged. Otherwise only messages from the specified category
 *                     will be logged at the specified level and above.
 * @param[in] level    Log level at which the log category is set.
 */
BITCOINKERNEL_API void kernel_add_log_level_category(const kernel_LogCategory category, kernel_LogLevel level);

/**
 * @brief Enable a specific log category for the global internal logger. This
 * function is not thread safe. Mutiple calls from different threads are
 * allowed but must be synchronized. This changes a global setting and will
 * override settings for all existing @ref kernel_LoggingConnection instances.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all categories will be enabled.
 */
BITCOINKERNEL_API void kernel_enable_log_category(const kernel_LogCategory category);

/**
 * @brief Disable a specific log category for the global internal logger. This
 * function is not thread safe. Mutiple calls from different threads are
 * allowed but must be synchronized. This changes a global setting and will
 * override settings for all existing @ref kernel_LoggingConnection instances.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all categories will be disabled.
 */
BITCOINKERNEL_API void kernel_disable_log_category(const kernel_LogCategory category);

/**
 * @brief Start logging messages through the provided callback. Log messages
 * produced before this function is first called are buffered and on calling this
 * function are logged immediately.
 *
 * @param[in] callback  Non-null, function through which messages will be logged.
 * @param[in] user_data Nullable, holds a user-defined opaque structure. Is passed back
 *                      to the user through the callback.
 * @param[in] options   Sets formatting options of the log messages.
 * @return              A new kernel logging connection, or null on error.
 */
BITCOINKERNEL_API kernel_LoggingConnection* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_logging_connection_create(
    kernel_LogCallback callback,
    const void* user_data,
    const kernel_LoggingOptions options
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Stop logging and destroy the logging connection.
 */
BITCOINKERNEL_API void kernel_logging_connection_destroy(kernel_LoggingConnection* logging_connection);

///@}

/** @name ChainParameters
 * Functions for working with chain parameters.
 */
///@{

/**
 * @brief Creates a chain parameters struct with default parameters based on the
 * passed in chain type.
 *
 * @param[in] chain_type Controls the chain parameters type created.
 * @return               An allocated chain parameters opaque struct.
 */
BITCOINKERNEL_API kernel_ChainParameters* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chain_parameters_create(
    const kernel_ChainType chain_type);

/**
 * Destroy the chain parameters.
 */
BITCOINKERNEL_API void kernel_chain_parameters_destroy(kernel_ChainParameters* chain_parameters);

///@}

/** @name ContextOptions
 * Functions for working with context options.
 */
///@{

/**
 * Creates an empty context options.
 */
BITCOINKERNEL_API kernel_ContextOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_context_options_create();

/**
 * @brief Sets the chain params for the context options. The context created
 * with the options will be configured for these chain parameters.
 *
 * @param[in] context_options  Non-null, previously created by @ref kernel_context_options_create.
 * @param[in] chain_parameters Is set to the context options.
 */
BITCOINKERNEL_API void kernel_context_options_set_chainparams(
    kernel_ContextOptions* context_options,
    const kernel_ChainParameters* chain_parameters
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Set the kernel notifications for the context options. The context
 * created with the options will be configured with these notifications.
 *
 * @param[in] context_options Non-null, previously created by @ref kernel_context_options_create.
 * @param[in] notifications   Is set to the context options.
 */
BITCOINKERNEL_API void kernel_context_options_set_notifications(
    kernel_ContextOptions* context_options,
    kernel_NotificationInterfaceCallbacks notifications
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the context options.
 */
BITCOINKERNEL_API void kernel_context_options_destroy(kernel_ContextOptions* context_options);

///@}

/** @name Context
 * Functions for working with contexts.
 */
///@{

/**
 * @brief Create a new kernel context. If the options have not been previously
 * set, their corresponding fields will be initialized to default values; the
 * context will assume mainnet chain parameters and won't attempt to call the
 * kernel notification callbacks.
 *
 * @param[in] context_options Nullable, created by @ref kernel_context_options_create.
 * @return                    The allocated kernel context, or null on error.
 */
BITCOINKERNEL_API kernel_Context* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_context_create(
    const kernel_ContextOptions* context_options);

/**
 * Destroy the context.
 */
BITCOINKERNEL_API void kernel_context_destroy(kernel_Context* context);

///@}

/** @name ChainstateManagerOptions
 * Functions for working with chainstate manager options.
 */
///@{

/**
 * @brief Create options for the chainstate manager.
 *
 * @param[in] context          Non-null, the created options will associate with this kernel context
 *                             for the duration of their lifetime. The same context needs to be used
 *                             when instantiating the chainstate manager.
 * @param[in] data_directory   Non-null, path string of the directory containing the chainstate data.
 *                             If the directory does not exist yet, it will be created.
 * @param[in] blocks_directory Non-null, path string of the directory containing the block data. If
 *                             the directory does not exist yet, it will be created.
 * @return                     The allocated chainstate manager options, or null on error.
 */
BITCOINKERNEL_API kernel_ChainstateManagerOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_options_create(
    const kernel_Context* context,
    const char* data_directory,
    size_t data_directory_len,
    const char* blocks_directory,
    size_t blocks_directory_len
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Set the number of available worker threads used during validation.
 *
 * @param[in] chainstate_manager_options Non-null, options to be set.
 * @param[in] worker_threads             The number of worker threads that should be spawned in the thread pool
 *                                       used for validation. When set to 0 no parallel verification is done.
 *                                       The value range is clamped internally between 0 and 15.
 */
BITCOINKERNEL_API void kernel_chainstate_manager_options_set_worker_threads_num(
        kernel_ChainstateManagerOptions* chainstate_manager_options,
        int worker_threads
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the chainstate manager options.
 */
BITCOINKERNEL_API void kernel_chainstate_manager_options_destroy(kernel_ChainstateManagerOptions* chainstate_manager_options);

///@}

/** @name ChainstateManager
 * Functions for chainstate management.
 */
///@{

/**
 * @brief Create a chainstate manager. This is the main object for many
 * validation tasks as well as for retrieving data from the chain and
 * interacting with its chainstate and indexes. It is only valid for as long as
 * the passed in context also remains in memory.
 *
 * @param[in] chainstate_manager_options Non-null, created by @ref kernel_chainstate_manager_options_create.
 * @param[in] context                    Non-null, the created chainstate manager will associate with this
 *                                       kernel context for the duration of its lifetime. The same context
 *                                       needs to be used for later interactions with the chainstate manager.
 * @return                               The allocated chainstate manager, or null on error.
 */
BITCOINKERNEL_API kernel_ChainstateManager* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_create(
    const kernel_Context* context,
    const kernel_ChainstateManagerOptions* chainstate_manager_options
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * Destroy the chainstate manager.
 */
BITCOINKERNEL_API void kernel_chainstate_manager_destroy(kernel_ChainstateManager* chainstate_manager, const kernel_Context* context);

///@}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BITCOIN_KERNEL_BITCOINKERNEL_H
