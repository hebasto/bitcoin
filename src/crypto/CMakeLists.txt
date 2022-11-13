# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

if(ASM AND NOT MSVC)
  # Check for SSE4.1 intrinsics.
  set(SSE41_CXXFLAGS -msse4.1)
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${SSE41_CXXFLAGS}")
  check_cxx_source_compiles("
    #include <immintrin.h>

    int main()
    {
      __m128i l = _mm_set1_epi32(0);
      return _mm_extract_epi32(l, 3);
    }
  " HAVE_SSE41)

  # Check for AVX2 intrinsics.
  set(AVX2_CXXFLAGS -mavx -mavx2)
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${AVX2_CXXFLAGS}")
  check_cxx_source_compiles("
    #include <immintrin.h>

    int main()
    {
      __m256i l = _mm256_set1_epi32(0);
      return _mm256_extract_epi32(l, 7);
    }
  " HAVE_AVX2)

  # Check for x86 SHA-NI intrinsics.
  set(X86_SHANI_CXXFLAGS -msse4 -msha)
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${X86_SHANI_CXXFLAGS}")
  check_cxx_source_compiles("
    #include <immintrin.h>

    int main()
    {
      __m128i i = _mm_set1_epi32(0);
      __m128i j = _mm_set1_epi32(1);
      __m128i k = _mm_set1_epi32(2);
      return _mm_extract_epi32(_mm_sha256rnds2_epu32(i, i, k), 0);
    }
  " HAVE_X86_SHANI)

  # Check for ARMv8 SHA-NI intrinsics.
  set(ARM_SHANI_CXXFLAGS -march=armv8-a+crypto)
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${ARM_SHANI_CXXFLAGS}")
  check_cxx_source_compiles("
    #include <arm_neon.h>

    int main()
    {
      uint32x4_t a, b, c;
      vsha256h2q_u32(a, b, c);
      vsha256hq_u32(a, b, c);
      vsha256su0q_u32(a, b);
      vsha256su1q_u32(a, b, c);
    }
  " HAVE_ARM_SHANI)

  unset(CMAKE_REQUIRED_FLAGS)
endif()

# The `bitcoin_crypto` must be of an `OBJECT` type to include
# its object files into the output static library archives.
add_library(bitcoin_crypto OBJECT EXCLUDE_FROM_ALL "")
target_sources(bitcoin_crypto
  PRIVATE
    $<$<BOOL:${ASM}>:sha256_sse4.cpp>
    $<$<BOOL:${HAVE_SSE41}>:sha256_sse41.cpp>
    $<$<BOOL:${HAVE_AVX2}>:sha256_avx2.cpp>
    $<$<BOOL:${HAVE_X86_SHANI}>:sha256_x86_shani.cpp>
    $<$<BOOL:${HAVE_ARM_SHANI}>:sha256_arm_shani.cpp>
    aes.cpp
    chacha_poly_aead.cpp
    chacha20.cpp
    hkdf_sha256_32.cpp
    hmac_sha256.cpp
    hmac_sha512.cpp
    poly1305.cpp
    muhash.cpp
    ripemd160.cpp
    sha1.cpp
    sha256.cpp
    sha3.cpp
    sha512.cpp
    siphash.cpp
)
target_compile_definitions(bitcoin_crypto
  PRIVATE
    $<$<BOOL:${ASM}>:USE_ASM>
    $<$<BOOL:${HAVE_SSE41}>:ENABLE_SSE41>
    $<$<BOOL:${HAVE_AVX2}>:ENABLE_AVX2>
    $<$<BOOL:${HAVE_X86_SHANI}>:ENABLE_X86_SHANI>
    $<$<BOOL:${HAVE_ARM_SHANI}>:ENABLE_ARM_SHANI>
)
target_compile_options(bitcoin_crypto
  PRIVATE
    $<$<BOOL:${HAVE_SSE41}>:${SSE41_CXXFLAGS}>
    $<$<BOOL:${HAVE_AVX2}>:${AVX2_CXXFLAGS}>
    $<$<BOOL:${HAVE_X86_SHANI}>:${X86_SHANI_CXXFLAGS}>
    $<$<BOOL:${HAVE_ARM_SHANI}>:${ARM_SHANI_CXXFLAGS}>
)
