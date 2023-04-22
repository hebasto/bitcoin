linux_CFLAGS=-pipe -std=$(C_STANDARD)
linux_CXXFLAGS=-pipe -std=$(CXX_STANDARD)

ifneq ($(LTO),)
linux_CFLAGS += -flto
linux_CXXFLAGS += -flto
linux_LDFLAGS += -flto

linux_AR = $(host_toolchain)gcc-ar
linux_NM = $(host_toolchain)gcc-nm
linux_RANLIB = $(host_toolchain)gcc-ranlib
endif

linux_release_CFLAGS=-O2
linux_release_CXXFLAGS=$(linux_release_CFLAGS)

linux_debug_CFLAGS=-O1
linux_debug_CXXFLAGS=$(linux_debug_CFLAGS)

linux_debug_CPPFLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_ENABLE_ASSERTIONS=1

i686_linux_CC = $(default_host_CC) -m32
i686_linux_CXX = $(default_host_CXX) -m32

x86_64_linux_CC = $(default_host_CC) -m64
x86_64_linux_CXX = $(default_host_CXX) -m64

linux_cmake_system=Linux
