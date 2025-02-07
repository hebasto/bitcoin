#ifndef BITCOIN_KERNEL_EXPORT_H
#define BITCOIN_KERNEL_EXPORT_H

#ifdef BITCOINKERNEL_STATIC
#  define BITCOINKERNEL_EXPORT
#else
#  ifndef BITCOINKERNEL_EXPORT
#    ifdef bitcoinkernel_EXPORTS
       /* We are building this library */
#      if defined(_WIN32)
#        define BITCOINKERNEL_EXPORT __declspec(dllexport)
#      endif
#    else
       /* We are using this library */
#      if defined(_WIN32)
#        define BITCOINKERNEL_EXPORT __declspec(dllimport)
#      endif
#    endif
#  endif
#endif

#endif // BITCOIN_KERNEL_EXPORT_H
