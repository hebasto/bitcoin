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
  LANGUAGES CXX ASM
)

set(PACKAGE_VERSION ${PROJECT_VERSION})
if(CLIENT_VERSION_RC GREATER 0)
  string(APPEND PACKAGE_VERSION "rc${CLIENT_VERSION_RC}")
endif()

set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_FINAL "The ${PACKAGE_NAME} developers")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/module)

# Configurable options.
# When adding a new option, end the <help_text> with a full stop for consistency.
option(BUILD_DAEMON "Build bitcoind executable." ON)
option(BUILD_CLI "Build bitcoin-cli executable." ON)
option(BUILD_TX "Build bitcoin-tx executable." ON)
option(BUILD_UTIL "Build bitcoin-util executable." ON)

option(ENABLE_WALLET "Enable wallet." ON)
# TODO: These tri-state options will be removed and most features
#       will become opt-in by default before merging into master.
include(TristateOption)
tristate_option(WITH_SQLITE "Enable SQLite wallet support." "if libsqlite3 is found." AUTO)
tristate_option(WITH_BDB "Enable Berkeley DB (BDB) wallet support." "if libdb_cxx is found." AUTO)
option(WARN_INCOMPATIBLE_BDB "Warn when using a Berkeley DB (BDB) version other than 4.8." ON)
include(CMakeDependentOption)
cmake_dependent_option(BUILD_WALLET_TOOL "Build bitcoin-wallet tool." ON "ENABLE_WALLET" OFF)

option(THREADLOCAL "Enable features that depend on the C++ thread_local keyword (currently just thread names in debug logs)." ON)
option(HARDENING "Attempt to harden the resulting executables." ON)
option(REDUCE_EXPORTS "Attempt to reduce exported symbols in the resulting executables." OFF)
option(WERROR "Treat compiler warnings as errors." OFF)

tristate_option(CCACHE "Use ccache for compiling." "if ccache is found." AUTO)
tristate_option(WITH_NATPMP "Enable NAT-PMP." "if libnatpmp is found." AUTO)
tristate_option(WITH_MINIUPNPC "Enable UPnP." "if libminiupnpc is found." AUTO)
tristate_option(WITH_ZMQ "Enable ZMQ notifications." "if libzmq is found." AUTO)
tristate_option(WITH_USDT
  "Enable tracepoints for Userspace, Statically Defined Tracing."
  "if sys/sdt.h is found."
  AUTO
)
tristate_option(WITH_EXTERNAL_SIGNER
  "Enable external signer support."
  "if Boost.Process is found."
  AUTO
)
tristate_option(MULTIPROCESS "Build multiprocess bitcoin-node, bitcoin-wallet, and bitcoin-gui executables in addition to monolithic bitcoind and bitcoin-qt executables. Requires libmultiprocess library. Experimental." "if libmultiprocess is found." OFF)

option(BUILD_TESTS "Build test_bitcoin executable." ON)
option(BUILD_BENCH "Build bench_bitcoin executable." ON)
cmake_dependent_option(BUILD_FUZZ_BINARY "Build fuzz binary." ON "NOT MSVC" OFF)
cmake_dependent_option(FUZZ "Build for fuzzing. Enabling this will disable all other targets and override BUILD_FUZZ_BINARY." OFF "NOT MSVC" OFF)

option(INSTALL_MAN "Install man pages." ON)

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

if(FUZZ)
  message(WARNING "FUZZ=ON will disable all other targets and force BUILD_FUZZ_BINARY=ON.")
  set(BUILD_DAEMON OFF)
  set(BUILD_CLI OFF)
  set(BUILD_TX OFF)
  set(BUILD_UTIL OFF)
  set(BUILD_UTIL_CHAINSTATE OFF)
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_WALLET_TOOL OFF)
  set(WITH_GUI OFF)
  set(WITH_NATPMP OFF)
  set(WITH_MINIUPNPC OFF)
  set(WITH_ZMQ OFF)
  set(BUILD_TESTS OFF)
  set(BUILD_BENCH OFF)
  set(BUILD_FUZZ_BINARY ON)

  target_compile_definitions(core_interface INTERFACE
    ABORT_ON_FAILED_ASSUME
  )
endif()

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

  target_compile_definitions(core_interface INTERFACE
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

    target_compile_definitions(core_interface INTERFACE
      _UNICODE;UNICODE
    )
    target_compile_options(core_interface INTERFACE
      /utf-8
      /Zc:__cplusplus
    )
    # Improve parallelism in MSBuild.
    # See: https://devblogs.microsoft.com/cppblog/improved-parallelism-in-msbuild/.
    list(APPEND CMAKE_VS_GLOBALS "UseMultiToolTask=true")

    try_append_cxx_flags("/W3" TARGET core_interface SKIP_LINK)
    try_append_cxx_flags("/wd4018" TARGET core_interface SKIP_LINK)
    try_append_cxx_flags("/wd4244" TARGET core_interface SKIP_LINK)
    try_append_cxx_flags("/wd4267" TARGET core_interface SKIP_LINK)
    try_append_cxx_flags("/wd4715" TARGET core_interface SKIP_LINK)
    try_append_cxx_flags("/wd4805" TARGET core_interface SKIP_LINK)
    target_compile_definitions(core_interface INTERFACE
      _CRT_SECURE_NO_WARNINGS
      _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    )
  endif()

  if(MINGW)
    target_compile_definitions(core_interface INTERFACE
      WIN32
      _WINDOWS
      _MT
    )
    # Avoid the use of aligned vector instructions when building for Windows.
    # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54412.
    try_append_cxx_flags("-Wa,-muse-unaligned-vector-move" TARGET core_interface SKIP_LINK)
    try_append_linker_flag("-static" TARGET core_interface)
    # We require Windows 7 (NT 6.1) or later.
    try_append_linker_flag("-Wl,--major-subsystem-version,6" TARGET core_interface)
    try_append_linker_flag("-Wl,--minor-subsystem-version,1" TARGET core_interface)
  endif()
endif()

# Use 64-bit off_t on 32-bit Linux.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  # Ensure 64-bit offsets are used for filesystem accesses for 32-bit compilation.
  target_compile_definitions(core_interface INTERFACE
    _FILE_OFFSET_BITS=64
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_definitions(core_interface INTERFACE
    MAC_OSX
  )
  # These flags are specific to ld64, and may cause issues with other linkers.
  # For example: GNU ld will interpret -dead_strip as -de and then try and use
  # "ad_strip" as the symbol for the entry point.
  try_append_linker_flag("-Wl,-dead_strip" TARGET core_interface)
  try_append_linker_flag("-Wl,-dead_strip_dylibs" TARGET core_interface)
  try_append_linker_flag("-Wl,-headerpad_max_install_names" TARGET core_interface)
endif()

if(CMAKE_CROSSCOMPILING)
  target_compile_definitions(core_interface INTERFACE ${DEPENDS_COMPILE_DEFINITIONS})
  target_compile_options(core_interface INTERFACE "$<$<COMPILE_LANGUAGE:C>:${DEPENDS_C_COMPILER_FLAGS}>")
  target_compile_options(core_interface INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:${DEPENDS_CXX_COMPILER_FLAGS}>")
endif()

include(AddThreadsIfNeeded)
add_threads_if_needed()

add_library(sanitizing_interface INTERFACE)
target_link_libraries(core_interface INTERFACE sanitizing_interface)
if(SANITIZERS)
  # First check if the compiler accepts flags. If an incompatible pair like
  # -fsanitize=address,thread is used here, this check will fail. This will also
  # fail if a bad argument is passed, e.g. -fsanitize=undfeined
  try_append_cxx_flags("-fsanitize=${SANITIZERS}" TARGET sanitizing_interface
    RESULT_VAR cxx_supports_sanitizers
    SKIP_LINK
  )
  if(NOT cxx_supports_sanitizers)
    message(FATAL_ERROR "Compiler did not accept requested flags.")
  endif()

  # Some compilers (e.g. GCC) require additional libraries like libasan,
  # libtsan, libubsan, etc. Make sure linking still works with the sanitize
  # flag. This is a separate check so we can give a better error message when
  # the sanitize flags are supported by the compiler but the actual sanitizer
  # libs are missing.
  try_append_linker_flag("-fsanitize=${SANITIZERS}" VAR SANITIZER_LDFLAGS
    SOURCE "
      #include <cstdint>
      #include <cstddef>
      extern \"C\" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) { return 0; }
      __attribute__((weak)) // allow for libFuzzer linking
      int main() { return 0; }
    "
    RESULT_VAR linker_supports_sanitizers
  )
  if(NOT linker_supports_sanitizers)
    message(FATAL_ERROR "Linker did not accept requested flags, you are missing required libraries.")
  endif()
endif()
target_link_options(sanitizing_interface INTERFACE ${SANITIZER_LDFLAGS})

include(AddBoostIfNeeded)
add_boost_if_needed()

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
target_compile_definitions(core_interface INTERFACE
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

include(cmake/optional_qt.cmake)

include(cmake/optional.cmake)

# Don't allow extended (non-ASCII) symbols in identifiers. This is easier for code review.
try_append_cxx_flags("-fno-extended-identifiers" TARGET core_interface)

# Currently all versions of gcc are subject to a class of bugs, see the
# gccbug_90348 test case (only reproduces on GCC 11 and earlier) and
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111843. To work around that, set
# -fstack-reuse=none for all gcc builds. (Only gcc understands this flag).
try_append_cxx_flags("-fstack-reuse=none" TARGET core_interface)

if(HARDENING)
  add_library(hardening_interface INTERFACE)
  target_link_libraries(core_interface INTERFACE hardening_interface)
  if(MSVC)
    try_append_linker_flag("/DYNAMICBASE" TARGET hardening_interface)
    try_append_linker_flag("/HIGHENTROPYVA" TARGET hardening_interface)
    try_append_linker_flag("/NXCOMPAT" TARGET hardening_interface)
  else()
    target_compile_options(hardening_interface INTERFACE
      $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3>
    )

    try_append_cxx_flags("-Wstack-protector" TARGET hardening_interface SKIP_LINK)
    try_append_cxx_flags("-fstack-protector-all" TARGET hardening_interface)
    try_append_cxx_flags("-fcf-protection=full" TARGET hardening_interface)

    if(MINGW)
      # stack-clash-protection doesn't compile with GCC 10 and earlier.
      # In any case, it is a no-op for Windows.
      # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90458 for more details.
    else()
      try_append_cxx_flags("-fstack-clash-protection" TARGET hardening_interface)
    endif()

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
      try_append_cxx_flags("-mbranch-protection=bti" TARGET hardening_interface)
    endif()

    try_append_linker_flag("-Wl,--enable-reloc-section" TARGET hardening_interface)
    try_append_linker_flag("-Wl,--dynamicbase" TARGET hardening_interface)
    try_append_linker_flag("-Wl,--nxcompat" TARGET hardening_interface)
    try_append_linker_flag("-Wl,--high-entropy-va" TARGET hardening_interface)
    try_append_linker_flag("-Wl,-z,relro" TARGET hardening_interface)
    try_append_linker_flag("-Wl,-z,now" TARGET hardening_interface)
    try_append_linker_flag("-Wl,-z,separate-code" TARGET hardening_interface)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      try_append_linker_flag("-Wl,-bind_at_load" TARGET hardening_interface)
      try_append_linker_flag("-Wl,-fixup_chains" TARGET hardening_interface)
    endif()
  endif()
endif()

if(REDUCE_EXPORTS)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
  try_append_linker_flag("-Wl,--exclude-libs,ALL" TARGET core_interface)
endif()

if(WERROR)
  if(MSVC)
    set(werror_flag "/WX")
  else()
    set(werror_flag "-Werror")
  endif()
  try_append_cxx_flags(${werror_flag} TARGET core_interface RESULT_VAR compiler_supports_werror SKIP_LINK)
  if(NOT compiler_supports_werror)
    message(FATAL_ERROR "WERROR set but ${werror_flag} is not usable.")
  endif()
  unset(werror_flag)
endif()

find_package(Python3 3.9 COMPONENTS Interpreter)
if(Python3_EXECUTABLE)
  set(PYTHON_COMMAND ${Python3_EXECUTABLE})
else()
  list(APPEND configure_warnings
    "Minimum required Python not found. Utils and rpcauth tests are disabled."
  )
endif()

add_subdirectory(src)
add_subdirectory(test)

include(cmake/tests.cmake)

include(Maintenance)
setup_split_debug_script()
add_maintenance_targets()
add_windows_deploy_target()

get_target_property(definitions core_interface INTERFACE_COMPILE_DEFINITIONS)
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
message("  GUI ................................. ${WITH_GUI}")
message("  external signer ..................... ${WITH_EXTERNAL_SIGNER}")
message("  multiprocess ........................ ${MULTIPROCESS}")
message("  NAT-PMP ............................. ${WITH_NATPMP}")
message("  UPnP ................................ ${WITH_MINIUPNPC}")
message("  ZeroMQ .............................. ${WITH_ZMQ}")
message("  USDT tracing ........................ ${WITH_USDT}")
message("Tests:")
message("  test_bitcoin ........................ ${BUILD_TESTS}")
message("  bench_bitcoin ....................... ${BUILD_BENCH}")
message("  fuzz binary ......................... ${BUILD_FUZZ_BINARY}")
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
include(GetTargetInterface)
get_target_interface(common_compile_options core_interface COMPILE_OPTIONS)
message("Common compile options ................ ${common_compile_options}")
get_target_interface(common_link_options core_interface LINK_OPTIONS)
message("Common link options ................... ${common_link_options}")
message("Linker flags for executables .......... ${CMAKE_EXE_LINKER_FLAGS}")
message("Linker flags for shared libraries ..... ${CMAKE_SHARED_LINKER_FLAGS}")
print_config_flags()
message("Attempt to harden executables ......... ${HARDENING}")
message("Treat compiler warnings as errors ..... ${WERROR}")
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
include(WarnAboutGlobalProperties)
