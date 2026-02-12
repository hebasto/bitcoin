OPENBSD_VERSION ?= 7.9
OPENBSD_SDK=$(SDK_PATH)/openbsd-$(host)-$(OPENBSD_VERSION)/

clang_prog=$(shell command -v clang)
clangxx_prog=$(shell command -v clang++)

openbsd_AR=$(shell command -v llvm-ar)
openbsd_NM=$(shell command -v llvm-nm)
openbsd_OBJCOPY=$(shell command -v llvm-objcopy)
openbsd_OBJDUMP=$(shell command -v llvm-objdump)
openbsd_RANLIB=$(shell command -v llvm-ranlib)
openbsd_STRIP=$(shell command -v llvm-strip)

openbsd_CC=$(clang_prog) --target=$(host) \
              --sysroot=$(OPENBSD_SDK)

openbsd_CXX=$(clangxx_prog) --target=$(host) \
              --sysroot=$(OPENBSD_SDK) -stdlib=libc++

openbsd_CFLAGS=
openbsd_CXXFLAGS=
openbsd_LDFLAGS=-fuse-ld=lld

openbsd_release_CFLAGS=-O2
openbsd_release_CXXFLAGS=$(openbsd_release_CFLAGS)

openbsd_debug_CFLAGS=-O1 -g
openbsd_debug_CXXFLAGS=$(openbsd_debug_CFLAGS)

openbsd_cmake_system_name=OpenBSD
