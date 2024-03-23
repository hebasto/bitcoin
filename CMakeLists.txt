# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Ubuntu 22.04 LTS Jammy Jellyfish, https://wiki.ubuntu.com/Releases, EOSS in June 2027:
#  - CMake 3.22.1, https://packages.ubuntu.com/jammy/cmake
#
# Centos Stream 9, EOL in May 2027:
#  - CMake 3.26.5, https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/
if(POLICY CMP0141)
  # MSVC debug information format flags are selected by an abstraction.
  # We want to use the CMAKE_MSVC_DEBUG_INFORMATION_FORMAT variable
  # to select the MSVC debug information format.
  cmake_policy(SET CMP0141 NEW)
endif()
cmake_minimum_required(VERSION 3.22)

#=============================
# Project / Package metadata
#=============================
set(PACKAGE_NAME "Bitcoin Core")
set(CLIENT_VERSION_MAJOR 27)
set(CLIENT_VERSION_MINOR 99)
set(CLIENT_VERSION_BUILD 0)
set(CLIENT_VERSION_RC 0)
set(CLIENT_VERSION_IS_RELEASE "false")
set(COPYRIGHT_YEAR "2024")

project(BitcoinCore
  VERSION ${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_BUILD}
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES NONE
)

set(PACKAGE_VERSION ${PROJECT_VERSION})
if(CLIENT_VERSION_RC GREATER 0)
  string(APPEND PACKAGE_VERSION "rc${CLIENT_VERSION_RC}")
endif()

set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_FINAL "The ${PACKAGE_NAME} developers")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")

#=============================
# Language setup
#=============================
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_HOST_APPLE)
  # We do not use the install_name_tool when cross-compiling for macOS.
  # So disable this tool check in further enable_language() commands.
  set(CMAKE_PLATFORM_HAS_INSTALLNAME FALSE)
endif()
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/module)

#=============================
# Configurable options
#=============================
# When adding a new option, end the <help_text> with a full stop for consistency.
option(BUILD_DAEMON "Build bitcoind executable." ON)
option(BUILD_CLI "Build bitcoin-cli executable." ON)

option(BUILD_TESTS "Build test_bitcoin executable." ON)
option(BUILD_TX "Build bitcoin-tx executable." ${BUILD_TESTS})
option(BUILD_UTIL "Build bitcoin-util executable." ${BUILD_TESTS})

option(ENABLE_WALLET "Enable wallet." ON)
# TODO: These tri-state options will be removed and most features
#       will become opt-in by default before merging into master.
include(TristateOption)
tristate_option(WITH_SQLITE "Enable SQLite wallet support." "if libsqlite3 is found." AUTO)
tristate_option(WITH_BDB "Enable Berkeley DB (BDB) wallet support." "if libdb_cxx is found." AUTO)
option(WARN_INCOMPATIBLE_BDB "Warn when using a Berkeley DB (BDB) version other than 4.8." ON)
include(CMakeDependentOption)
cmake_dependent_option(BUILD_WALLET_TOOL "Build bitcoin-wallet tool." ${BUILD_TESTS} "ENABLE_WALLET" OFF)

tristate_option(WITH_CCACHE "Use ccache for compiling." "if ccache is found." AUTO)
tristate_option(WITH_NATPMP "Enable NAT-PMP." "if libnatpmp is found." AUTO)
tristate_option(WITH_MINIUPNPC "Enable UPnP." "if libminiupnpc is found." AUTO)
tristate_option(WITH_ZMQ "Enable ZMQ notifications." "if libzmq is found." AUTO)
tristate_option(WITH_USDT
  "Enable tracepoints for Userspace, Statically Defined Tracing."
  "if sys/sdt.h is found."
  AUTO
)

option(BUILD_BENCH "Build bench_bitcoin executable." OFF)

set(configure_warnings)

include(CheckPIESupported)
check_pie_supported(OUTPUT_VARIABLE check_pie_output LANGUAGES CXX)
if(CMAKE_CXX_LINK_PIE_SUPPORTED)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
elseif(NOT WIN32)
  # The warning is superfluous for Windows.
  message(WARNING "PIE is not supported at link time: ${check_pie_output}")
  list(APPEND configure_warnings "Position independent code disabled.")
endif()
unset(check_pie_output)

# The core_interface library aims to encapsulate common build flags.
# It is intended to be a usage requirement for all other targets.
add_library(core_interface INTERFACE)
# The core_base_interface library is a subset of the core_interface
# designated to be used with subtrees. In particular, it does not
# include the warn_interface as subtree's warnings are not fixable
# in our tree.
add_library(core_base_interface INTERFACE)
add_library(core_depends_release_interface INTERFACE)
add_library(core_depends_debug_interface INTERFACE)
target_link_libraries(core_base_interface INTERFACE
  $<$<CONFIG:RelWithDebInfo>:core_depends_release_interface>
  $<$<CONFIG:Debug>:core_depends_debug_interface>
)
target_link_libraries(core_interface INTERFACE core_base_interface)

include(TryAppendCXXFlags)

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

  target_compile_definitions(core_base_interface INTERFACE
    _WIN32_WINNT=0x0601
    _WIN32_IE=0x0501
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )

  if(MSVC)
    if(VCPKG_TARGET_TRIPLET MATCHES "-static")
      set(msvc_library_linkage "")
    else()
      set(msvc_library_linkage "DLL")
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>${msvc_library_linkage}")
    unset(msvc_library_linkage)

    target_compile_definitions(core_base_interface INTERFACE
      _UNICODE;UNICODE
    )
    target_compile_options(core_interface INTERFACE
      /utf-8
      /Zc:preprocessor
      /Zc:__cplusplus
      /sdl
    )
    # Improve parallelism in MSBuild.
    # See: https://devblogs.microsoft.com/cppblog/improved-parallelism-in-msbuild/.
    list(APPEND CMAKE_VS_GLOBALS "UseMultiToolTask=true")
  endif()

  if(MINGW)
    target_compile_definitions(core_base_interface INTERFACE
      WIN32
      _WINDOWS
      _MT
    )
    # Avoid the use of aligned vector instructions when building for Windows.
    # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54412.
    try_append_cxx_flags("-Wa,-muse-unaligned-vector-move" TARGET core_base_interface SKIP_LINK)
  endif()
endif()

# Use 64-bit off_t on 32-bit Linux.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  # Ensure 64-bit offsets are used for filesystem accesses for 32-bit compilation.
  target_compile_definitions(core_base_interface INTERFACE
    _FILE_OFFSET_BITS=64
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_definitions(core_base_interface INTERFACE
    MAC_OSX
  )
endif()

include(AddThreadsIfNeeded)
add_threads_if_needed()

include(AddBoostIfNeeded)
add_boost_if_needed()

if(BUILD_DAEMON OR BUILD_CLI OR BUILD_TESTS OR BUILD_BENCH)
  find_package(Libevent 2.1.8 MODULE REQUIRED)
endif()

include(cmake/introspection.cmake)

include(cmake/crc32c.cmake)
include(cmake/leveldb.cmake)
include(cmake/minisketch.cmake)
include(cmake/secp256k1.cmake)

include(cmake/optional.cmake)

find_package(Python3 3.9 COMPONENTS Interpreter)
if(Python3_EXECUTABLE)
  set(PYTHON_COMMAND ${Python3_EXECUTABLE})
else()
  list(APPEND configure_warnings
    "Minimum required Python not found. Utils and rpcauth tests are disabled."
  )
endif()

target_compile_definitions(core_base_interface INTERFACE ${DEPENDS_COMPILE_DEFINITIONS})
target_compile_definitions(core_depends_release_interface INTERFACE ${DEPENDS_COMPILE_DEFINITIONS_RELWITHDEBINFO})
target_compile_definitions(core_depends_debug_interface INTERFACE ${DEPENDS_COMPILE_DEFINITIONS_DEBUG})

add_subdirectory(src)
add_subdirectory(test)

include(cmake/tests.cmake)


include(GetTargetInterface)
get_target_interface(definitions core_interface COMPILE_DEFINITIONS)
get_target_interface(definitions_RELWITHDEBINFO core_depends_release_interface COMPILE_DEFINITIONS)
get_target_interface(definitions_DEBUG core_depends_debug_interface COMPILE_DEFINITIONS)

message("\n")
message("Configure summary")
message("=================")
message("Executables:")
message("  bitcoind ............................ ${BUILD_DAEMON}")
message("  bitcoin-cli ......................... ${BUILD_CLI}")
message("  bitcoin-tx .......................... ${BUILD_TX}")
message("  bitcoin-util ........................ ${BUILD_UTIL}")
message("  bitcoin-wallet ...................... ${BUILD_WALLET_TOOL}")
message("Optional features:")
message("  wallet support ...................... ${ENABLE_WALLET}")
if(ENABLE_WALLET)
  message("   - descriptor wallets (SQLite) ...... ${WITH_SQLITE}")
  message("   - legacy wallets (Berkeley DB) ..... ${WITH_BDB}")
endif()
message("  port mapping:")
message("   - using NAT-PMP .................... ${WITH_NATPMP}")
message("   - using UPnP ....................... ${WITH_MINIUPNPC}")
message("  ZeroMQ .............................. ${WITH_ZMQ}")
message("  USDT tracing ........................ ${WITH_USDT}")
message("Tests:")
message("  test_bitcoin ........................ ${BUILD_TESTS}")
message("  bench_bitcoin ....................... ${BUILD_BENCH}")
message("")
if(CMAKE_CROSSCOMPILING)
  set(cross_status "TRUE, for ${CMAKE_SYSTEM_NAME}, ${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(cross_status "FALSE")
endif()
message("Cross compiling ....................... ${cross_status}")
message("Preprocessor defined macros ........... ${definitions}")
message("C compiler ............................ ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}, ${CMAKE_C_COMPILER}")
message("CFLAGS ................................ ${CMAKE_C_FLAGS}")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, ${CMAKE_CXX_COMPILER}")
message("CXXFLAGS .............................. ${CMAKE_CXX_FLAGS}")
get_target_interface(common_compile_options core_interface COMPILE_OPTIONS)
message("Common compile options ................ ${common_compile_options}")
get_target_interface(common_link_options core_interface LINK_OPTIONS)
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
message("Use ccache for compiling .............. ${WITH_CCACHE}")
message("\n")
if(configure_warnings)
    message("  ******\n")
    foreach(warning IN LISTS configure_warnings)
      message(WARNING "${warning}")
    endforeach()
    message("  ******\n")
endif()

# We want all build properties to be encapsulated properly.
include(WarnAboutGlobalProperties)
