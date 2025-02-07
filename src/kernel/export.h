#ifndef BITCOIN_KERNEL_EXPORT_H
#define BITCOIN_KERNEL_EXPORT_H

#ifndef BITCOINKERNEL_STATIC
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

#ifndef BITCOINKERNEL_EXPORT
#  define BITCOINKERNEL_EXPORT
#endif

#endif // BITCOIN_KERNEL_EXPORT_H
