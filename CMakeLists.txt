# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# IMPORTANT: Changes which affect binary results may not be quietly gated
#            by CMake version.
#
# Ubuntu 20.04 LTS Focal Fossa, https://wiki.ubuntu.com/Releases, EOSS in April 2025:
#  - CMake 3.16.3, https://packages.ubuntu.com/focal/cmake
#
# Centos Stream 8, EOL in May 2024:
#  - CMake 3.20.2, http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/
#
# All policies known to the running version of CMake and introduced
# in the 3.28 version or earlier will be set to use NEW behavior.
# All policies introduced in later versions will be unset.
# See: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
cmake_minimum_required(VERSION 3.16...3.28)

project("Bitcoin Core"
  VERSION 26.99.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX ASM
)

set(CLIENT_VERSION_IS_RELEASE "false")
set(COPYRIGHT_YEAR "2023")
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_FINAL "The ${PROJECT_NAME} developers")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/module)

# Configurable options.
# When adding a new option, end the <help_text> with a full stop for consistency.
include(CMakeDependentOption)
option(BUILD_DAEMON "Build bitcoind executable." ON)
option(BUILD_CLI "Build bitcoin-cli executable." ON)
option(BUILD_TX "Build bitcoin-tx executable." ON)
option(ASM "Use assembly routines." ON)
cmake_dependent_option(CXX20 "Enable compilation in C++20 mode." OFF "NOT MSVC" ON)
option(THREADLOCAL "Enable features that depend on the C++ thread_local keyword (currently just thread names in debug logs)." ON)

# TODO: These tri-state options will be removed and most features
#       will become opt-in by default before merging into master.
include(TristateOption)
tristate_option(CCACHE "Use ccache for compiling." "if ccache is found." AUTO)
tristate_option(WITH_NATPMP "Enable NAT-PMP." "if libnatpmp is found." AUTO)
tristate_option(WITH_MINIUPNPC "Enable UPnP." "if libminiupnpc is found." AUTO)
tristate_option(WITH_ZMQ "Enable ZMQ notifications." "if libzmq is found." AUTO)
tristate_option(WITH_USDT
  "Enable tracepoints for Userspace, Statically Defined Tracing."
  "if sys/sdt.h is found."
  AUTO
)

if(CXX20)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(configure_warnings)

include(CheckPIESupported)
check_pie_supported(OUTPUT_VARIABLE check_pie_output LANGUAGES CXX)
if(CMAKE_CXX_LINK_PIE_SUPPORTED)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
  message(WARNING "PIE is not supported at link time: ${check_pie_output}")
  list(APPEND configure_warnings "Position independent code disabled.")
endif()
unset(check_pie_output)

# The core interface library aims to encapsulate all common build flags.
# It is intended to be a usage requirement for all other targets.
add_library(core INTERFACE)

if(WIN32)
  #[=[
  This build system supports two ways to build binaries for Windows.

  1. Building on Windows using MSVC.
  Implementation notes:
  - /DWIN32 and /D_WINDOWS definitions are included into the CMAKE_CXX_FLAGS_INIT
    and CMAKE_CXX_FLAGS_INIT variables by default.
  - A run-time library is selected using the CMAKE_MSVC_RUNTIME_LIBRARY variable.
  - MSVC-specific options, for example, /Zc:__cplusplus, are additionally required.

  2. Cross-compiling using MinGW.
  Implementation notes:
  - WIN32 and _WINDOWS definitions must be provided explicitly.
  - A run-time library must be specified explicitly using _MT definition.
  ]=]

  target_compile_definitions(core INTERFACE
    _WIN32_WINNT=0x0601
    _WIN32_IE=0x0501
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )

  if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(core INTERFACE
      /utf-8
      /Zc:__cplusplus
    )
    # Improve parallelism in MSBuild.
    # See: https://devblogs.microsoft.com/cppblog/improved-parallelism-in-msbuild/.
    list(APPEND CMAKE_VS_GLOBALS "UseMultiToolTask=true")
  endif()

  if(MINGW)
    target_compile_definitions(core INTERFACE
      WIN32
      _WINDOWS
      _MT
    )
  endif()
endif()

# Use 64-bit off_t on 32-bit Linux.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  # Ensure 64-bit offsets are used for filesystem accesses for 32-bit compilation.
  target_compile_definitions(core INTERFACE
    _FILE_OFFSET_BITS=64
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_definitions(core INTERFACE
    MAC_OSX
  )
endif()

if(CMAKE_CROSSCOMPILING)
  target_compile_definitions(core INTERFACE ${DEPENDS_COMPILE_DEFINITIONS})
  target_compile_options(core INTERFACE "$<$<COMPILE_LANGUAGE:C>:${DEPENDS_C_COMPILER_FLAGS}>")
  target_compile_options(core INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:${DEPENDS_CXX_COMPILER_FLAGS}>")
  if(DEPENDS_ALLOW_HOST_PACKAGES)
    list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_SYSTEM_PREFIX_PATH}")
  endif()
endif()

include(AddThreadsIfNeeded)
add_threads_if_needed()

include(AddBoostIfNeeded)
add_boost_if_needed()

include(CheckSourceCompilesAndLinks)

include(AddLibeventIfNeeded)
add_libevent_if_needed()

include(cmake/introspection.cmake)

include(cmake/crc32c.cmake)
include(cmake/leveldb.cmake)
include(cmake/minisketch.cmake)
include(cmake/secp256k1.cmake)

include(cmake/optional.cmake)

add_subdirectory(src)

message("\n")
message("Configure summary")
message("=================")
message("Executables:")
message("  bitcoind ............................ ${BUILD_DAEMON}")
message("  bitcoin-cli ......................... ${BUILD_CLI}")
message("  bitcoin-tx .......................... ${BUILD_TX}")
message("Optional packages:")
message("  NAT-PMP ............................. ${WITH_NATPMP}")
message("  UPnP ................................ ${WITH_MINIUPNPC}")
message("  ZeroMQ .............................. ${WITH_ZMQ}")
message("  USDT tracing ........................ ${WITH_USDT}")
message("")
if(CMAKE_CROSSCOMPILING)
  set(cross_status "TRUE, for ${CMAKE_SYSTEM_NAME}, ${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(cross_status "FALSE")
endif()
message("Cross compiling ....................... ${cross_status}")
get_directory_property(definitions COMPILE_DEFINITIONS)
string(REPLACE ";" " " definitions "${definitions}")
message("Preprocessor defined macros ........... ${definitions}")
message("C compiler ............................ ${CMAKE_C_COMPILER}")
list(JOIN DEPENDS_C_COMPILER_FLAGS " " depends_c_flags)
string(STRIP "${CMAKE_C_FLAGS} ${depends_c_flags}" combined_c_flags)
message("CFLAGS ................................ ${combined_c_flags}")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER}")
list(JOIN DEPENDS_CXX_COMPILER_FLAGS " " depends_cxx_flags)
string(STRIP "${CMAKE_CXX_FLAGS} ${depends_cxx_flags}" combined_cxx_flags)
message("CXXFLAGS .............................. ${combined_cxx_flags}")
get_target_property(common_compile_options core INTERFACE_COMPILE_OPTIONS)
if(common_compile_options)
  list(JOIN common_compile_options " " common_compile_options)
else()
  set(common_compile_options)
endif()
string(GENEX_STRIP "${common_compile_options}" common_compile_options)
message("Common compile options ................ ${common_compile_options}")
get_target_property(common_link_options core INTERFACE_LINK_OPTIONS)
if(common_link_options)
  list(JOIN common_link_options " " common_link_options)
else()
  set(common_link_options)
endif()
message("Common link options ................... ${common_link_options}")
message("Linker flags for executables .......... ${CMAKE_EXE_LINKER_FLAGS}")
message("Linker flags for shared libraries ..... ${CMAKE_SHARED_LINKER_FLAGS}")
if(DEFINED CMAKE_BUILD_TYPE)
  message("Build type:")
  message(" - CMAKE_BUILD_TYPE ................... ${CMAKE_BUILD_TYPE}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_${build_type}}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_${build_type}}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_${build_type}}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_${build_type}}")
else()
  message("Available configurations .............. ${CMAKE_CONFIGURATION_TYPES}")
  message("Debug configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_DEBUG}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_DEBUG}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
  message("Release configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_RELEASE}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_RELEASE}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
endif()
message("Use assembly routines ................. ${ASM}")
message("Use ccache for compiling .............. ${CCACHE}")
message("\n")
if(configure_warnings)
    message("  ******\n")
    foreach(warning IN LISTS configure_warnings)
      message(WARNING "${warning}")
    endforeach()
    message("  ******\n")
endif()

# We want all build properties to be encapsulated properly.
get_directory_property(global_compile_definitions COMPILE_DEFINITIONS)
if(global_compile_definitions)
  message(AUTHOR_WARNING "The directory's COMPILE_DEFINITIONS property is not empty: ${global_compile_definitions}")
endif()
get_directory_property(global_compile_options COMPILE_OPTIONS)
if(global_compile_options)
  message(AUTHOR_WARNING "The directory's COMPILE_OPTIONS property is not empty: ${global_compile_options}")
endif()
get_directory_property(global_link_options LINK_OPTIONS)
if(global_link_options)
  message(AUTHOR_WARNING "The directory's LINK_OPTIONS property is not empty: ${global_link_options}")
endif()
