# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# This file is part of the transition from Autotools to CMake. Once CMake
# support has been merged we should switch to using the upstream CMake
# buildsystem.

enable_language(C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS OFF)
string(APPEND CMAKE_C_COMPILE_OBJECT " ${APPEND_CPPFLAGS} ${APPEND_CFLAGS}")

include(CheckCSourceCompiles)
check_c_source_compiles("
  #include <stdint.h>

  int main()
  {
    uint64_t a = 11, tmp;
    __asm__ __volatile__(\"movq $0x100000000,%1; mulq %%rsi\" : \"+a\"(a) : \"S\"(tmp) : \"cc\", \"%rdx\");
  }
  " HAVE_64BIT_ASM
)

add_library(secp256k1 STATIC EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/src/secp256k1/src/secp256k1.c
  ${PROJECT_SOURCE_DIR}/src/secp256k1/src/precomputed_ecmult.c
  ${PROJECT_SOURCE_DIR}/src/secp256k1/src/precomputed_ecmult_gen.c
)

target_compile_definitions(secp256k1
  PRIVATE
    ECMULT_WINDOW_SIZE=15
    ENABLE_MODULE_RECOVERY
    ENABLE_MODULE_SCHNORRSIG
    ENABLE_MODULE_EXTRAKEYS
    ENABLE_MODULE_ELLSWIFT
    # COMB_* definitions for SECP256K1_ECMULT_GEN_KB=86
    COMB_BLOCKS=43
    COMB_TEETH=6
    $<$<BOOL:${HAVE_64BIT_ASM}>:USE_ASM_X86_64=1>
  INTERFACE
    $<$<PLATFORM_ID:Windows>:SECP256K1_STATIC>
)

target_include_directories(secp256k1
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/secp256k1/include>
)

if(MSVC)
  target_compile_options(secp256k1
    PRIVATE
      /wd4146
      /wd4244
      /wd4267
  )
endif()

target_link_libraries(secp256k1 PRIVATE core_base_interface)
set_target_properties(secp256k1 PROPERTIES EXPORT_COMPILE_COMMANDS OFF)
