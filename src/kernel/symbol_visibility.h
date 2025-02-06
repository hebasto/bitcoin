#ifndef BITCOIN_KERNEL_SYMBOL_VISIBILITY_H
#define BITCOIN_KERNEL_SYMBOL_VISIBILITY_H

#if defined(_WIN32)
  #if defined(bitcoinkernel_EXPORTS)
    #define BITCOINKERNEL_EXPORT_SYMBOL __declspec(dllexport)
  #else
    #define BITCOINKERNEL_EXPORT_SYMBOL __declspec(dllimport)
  #endif
#else
  #define BITCOINKERNEL_EXPORT_SYMBOL
#endif

#endif // BITCOIN_KERNEL_SYMBOL_VISIBILITY_H
