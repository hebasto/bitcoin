openbsd_CFLAGS=-pipe -std=$(C_STANDARD)
openbsd_CXXFLAGS=-pipe -std=$(CXX_STANDARD)

openbsd_release_CFLAGS=-O2
openbsd_release_CXXFLAGS=$(openbsd_release_CFLAGS)

openbsd_debug_CFLAGS=-O1 -g
openbsd_debug_CXXFLAGS=$(openbsd_debug_CFLAGS)

ifeq ($(host),$(build))
openbsd_CC=clang
openbsd_CXX=clang++
else
ifeq (86,$(findstring 86,$(build_arch)))
i686_openbsd_CC=clang -m32
i686_openbsd_CXX=clang++ -m32

x86_64_openbsd_CC=clang -m64
x86_64_openbsd_CXX=clang++ -m64
endif
endif

openbsd_AR=ar
openbsd_RANLIB=ranlib
openbsd_NM=nm
openbsd_STRIP=strip

openbsd_cmake_system_name=OpenBSD
