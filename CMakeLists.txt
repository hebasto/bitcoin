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

set(PACKAGE_NAME ${PROJECT_NAME})
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
option(BUILD_UTIL "Build bitcoin-util executable." ON)
option(ASM "Use assembly routines." ON)

option(ENABLE_WALLET "Enable wallet." ON)
# TODO: These tri-state options will be removed and most features
#       will become opt-in by default before merging into master.
include(TristateOption)
tristate_option(WITH_SQLITE "Enable SQLite wallet support." "if libsqlite3 is found." AUTO)
tristate_option(WITH_BDB "Enable Berkeley DB (BDB) wallet support." "if libdb_cxx is found." AUTO)
option(WARN_INCOMPATIBLE_BDB "Warn when using a Berkeley DB (BDB) version other than 4.8." ON)
cmake_dependent_option(BUILD_WALLET_TOOL "Build bitcoin-wallet tool." ON "ENABLE_WALLET" OFF)

cmake_dependent_option(CXX20 "Enable compilation in C++20 mode." OFF "NOT MSVC" ON)
option(THREADLOCAL "Enable features that depend on the C++ thread_local keyword (currently just thread names in debug logs)." ON)
option(HARDENING "Attempt to harden the resulting executables." ON)

tristate_option(CCACHE "Use ccache for compiling." "if ccache is found." AUTO)
tristate_option(WITH_NATPMP "Enable NAT-PMP." "if libnatpmp is found." AUTO)
tristate_option(WITH_MINIUPNPC "Enable UPnP." "if libminiupnpc is found." AUTO)
tristate_option(WITH_ZMQ "Enable ZMQ notifications." "if libzmq is found." AUTO)
tristate_option(WITH_USDT
  "Enable tracepoints for Userspace, Statically Defined Tracing."
  "if sys/sdt.h is found."
  AUTO
)

option(BUILD_TESTS "Build test_bitcoin executable." ON)
option(BUILD_BENCH "Build bench_bitcoin executable." ON)

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

include(TryAppendCXXFlags)
include(TryAppendLinkerFlag)

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
    try_append_linker_flag("-static" TARGET core)
    # We require Windows 7 (NT 6.1) or later.
    try_append_linker_flag("-Wl,--major-subsystem-version,6" TARGET core)
    try_append_linker_flag("-Wl,--minor-subsystem-version,1" TARGET core)
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
  # These flags are specific to ld64, and may cause issues with other linkers.
  # For example: GNU ld will interpret -dead_strip as -de and then try and use
  # "ad_strip" as the symbol for the entry point.
  try_append_linker_flag("-Wl,-dead_strip" TARGET core)
  try_append_linker_flag("-Wl,-dead_strip_dylibs" TARGET core)
  try_append_linker_flag("-Wl,-headerpad_max_install_names" TARGET core)
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

include(ProcessConfigurations)
set_default_config(RelWithDebInfo)

# Redefine configuration flags.
target_compile_definitions(core INTERFACE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Debug>:DEBUG_LOCKORDER>
  $<$<CONFIG:Debug>:DEBUG_LOCKCONTENTION>
  $<$<CONFIG:Debug>:RPC_DOC_CHECK>
  $<$<CONFIG:Debug>:ABORT_ON_FAILED_ASSUME>
)
# We leave assertions on.
if(MSVC)
  remove_c_flag_from_all_configs(/DNDEBUG)
  remove_cxx_flag_from_all_configs(/DNDEBUG)
else()
  remove_c_flag_from_all_configs(-DNDEBUG)
  remove_cxx_flag_from_all_configs(-DNDEBUG)

  # Prefer -O2 optimization level. (-O3 is CMake's default for Release for many compilers.)
  replace_c_flag_in_config(Release -O3 -O2)
  replace_cxx_flag_in_config(Release -O3 -O2)

  set(debug_flags)
  try_append_cxx_flags("-O0" VAR debug_flags)
  try_append_cxx_flags("-g3" VAR debug_flags RESULT_VAR compiler_supports_g3)
  if(NOT compiler_supports_g3)
    try_append_cxx_flags("-g" VAR debug_flags)
  endif()
  set(CMAKE_C_FLAGS_DEBUG "${debug_flags}")
  try_append_cxx_flags("-ftrapv" VAR debug_flags)
  set(CMAKE_CXX_FLAGS_DEBUG "${debug_flags}")
  unset(debug_flags)
endif()

include(cmake/optional.cmake)

# Don't allow extended (non-ASCII) symbols in identifiers. This is easier for code review.
try_append_cxx_flags("-fno-extended-identifiers" TARGET core)

# Currently all versions of gcc are subject to a class of bugs, see the
# gccbug_90348 test case (only reproduces on GCC 11 and earlier) and
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111843. To work around that, set
# -fstack-reuse=none for all gcc builds. (Only gcc understands this flag).
try_append_cxx_flags("-fstack-reuse=none" TARGET core)

if(HARDENING)
  add_library(hardening INTERFACE)
  if(MSVC)
    try_append_linker_flag("/DYNAMICBASE" TARGET hardening)
    try_append_linker_flag("/HIGHENTROPYVA" TARGET hardening)
    try_append_linker_flag("/NXCOMPAT" TARGET hardening)
  else()
    target_compile_options(hardening INTERFACE
      $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3>
    )

    try_append_cxx_flags("-Wstack-protector" TARGET hardening)
    try_append_cxx_flags("-fstack-protector-all" TARGET hardening)
    try_append_cxx_flags("-fcf-protection=full" TARGET hardening)

    if(MINGW)
      # stack-clash-protection doesn't compile with GCC 10 and earlier.
      # In any case, it is a no-op for Windows.
      # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90458 for more details.
    else()
      try_append_cxx_flags("-fstack-clash-protection" TARGET hardening)
    endif()

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
      try_append_cxx_flags("-mbranch-protection=bti" TARGET hardening)
    endif()

    try_append_linker_flag("-Wl,--enable-reloc-section" TARGET hardening)
    try_append_linker_flag("-Wl,--dynamicbase" TARGET hardening)
    try_append_linker_flag("-Wl,--nxcompat" TARGET hardening)
    try_append_linker_flag("-Wl,--high-entropy-va" TARGET hardening)
    try_append_linker_flag("-Wl,-z,relro" TARGET hardening)
    try_append_linker_flag("-Wl,-z,now" TARGET hardening)
    try_append_linker_flag("-Wl,-z,separate-code" TARGET hardening)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      try_append_linker_flag("-Wl,-bind_at_load" TARGET hardening)
      try_append_linker_flag("-Wl,-fixup_chains" TARGET hardening)
    endif()
  endif()
  target_link_libraries(core INTERFACE hardening)
endif()

find_package(Python3 3.9 COMPONENTS Interpreter)
set(PYTHON_COMMAND ${Python3_EXECUTABLE})

add_subdirectory(src)
add_subdirectory(test)

include(cmake/tests.cmake)

get_target_property(definitions core INTERFACE_COMPILE_DEFINITIONS)
separate_by_configs(definitions)

message("\n")
message("Configure summary")
message("=================")
message("Executables:")
message("  bitcoind ............................ ${BUILD_DAEMON}")
message("  bitcoin-cli ......................... ${BUILD_CLI}")
message("  bitcoin-tx .......................... ${BUILD_TX}")
message("  bitcoin-util ........................ ${BUILD_UTIL}")
message("  bitcoin-wallet ...................... ${BUILD_WALLET_TOOL}")
message("Wallet support:")
message("  SQLite, descriptor wallets .......... ${WITH_SQLITE}")
message("  Berkeley DB, legacy wallets ......... ${WITH_BDB}")
message("Optional packages:")
message("  NAT-PMP ............................. ${WITH_NATPMP}")
message("  UPnP ................................ ${WITH_MINIUPNPC}")
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
message("Preprocessor defined macros ........... ${definitions_ALL}")
message("C compiler ............................ ${CMAKE_C_COMPILER}")
list(JOIN DEPENDS_C_COMPILER_FLAGS " " depends_c_flags)
string(STRIP "${CMAKE_C_FLAGS} ${depends_c_flags}" combined_c_flags)
message("CFLAGS ................................ ${combined_c_flags}")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER}")
list(JOIN DEPENDS_CXX_COMPILER_FLAGS " " depends_cxx_flags)
string(STRIP "${CMAKE_CXX_FLAGS} ${depends_cxx_flags}" combined_cxx_flags)
message("CXXFLAGS .............................. ${combined_cxx_flags}")
get_target_interface(common_compile_options core COMPILE_OPTIONS)
message("Common compile options ................ ${common_compile_options}")
get_target_interface(common_link_options core LINK_OPTIONS)
message("Common link options ................... ${common_link_options}")
message("Linker flags for executables .......... ${CMAKE_EXE_LINKER_FLAGS}")
message("Linker flags for shared libraries ..... ${CMAKE_SHARED_LINKER_FLAGS}")
print_config_flags()
message("Use assembly routines ................. ${ASM}")
message("Attempt to harden executables ......... ${HARDENING}")
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
