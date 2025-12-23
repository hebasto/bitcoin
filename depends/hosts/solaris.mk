solaris_CFLAGS=
solaris_CXXFLAGS=

ifneq ($(LTO),)
solaris_AR = $(host_toolchain)gcc-ar
solaris_NM = $(host_toolchain)gcc-nm
solaris_RANLIB = $(host_toolchain)gcc-ranlib
endif

solaris_release_CFLAGS=-O2
solaris_release_CXXFLAGS=$(netbsd_release_CFLAGS)

solaris_debug_CFLAGS=-O1 -g
solaris_debug_CXXFLAGS=$(netbsd_debug_CFLAGS)

ifeq (86,$(findstring 86,$(build_arch)))
i686_solaris_CC=gcc -m32
i686_solaris_CXX=g++ -m32
i686_solaris_AR=ar
i686_solaris_RANLIB=ranlib
i686_solaris_NM=nm
i686_solaris_STRIP=strip

x86_64_solaris_CC=gcc -m64
x86_64_solaris_CXX=g++ -m64
x86_64_solaris_AR=ar
x86_64_solaris_RANLIB=ranlib
x86_64_solaris_NM=nm
x86_64_solaris_STRIP=strip
else
i686_solaris_CC=$(default_host_CC) -m32
i686_solaris_CXX=$(default_host_CXX) -m32
x86_64_solaris_CC=$(default_host_CC) -m64
x86_64_solaris_CXX=$(default_host_CXX) -m64
endif

solaris_cmake_system_name=SunOS
