#ifndef BITCOIN_KERNEL_SYMBOL_VISIBILITY_H
#define BITCOIN_KERNEL_SYMBOL_VISIBILITY_H

#if defined(bitcoinkernel_EXPORTS)
  #if defined(_WIN32)
    #define BITCOINKERNEL_EXPORT_SYMBOL __declspec(dllexport)
  #else
    #define BITCOINKERNEL_EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(_WIN32) && !defined(STATIC_LIBBITCOINKERNEL)
  #define BITCOINKERNEL_EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef BITCOINKERNEL_EXPORT_SYMBOL
  #define BITCOINKERNEL_EXPORT_SYMBOL
#endif

#endif // BITCOIN_KERNEL_SYMBOL_VISIBILITY_H
