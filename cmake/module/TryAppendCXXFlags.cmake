# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)

#[=[
Usage examples:

  try_append_cxx_flags("-fsanitize=${SANITIZERS}" TARGET core_interface
    RESULT_VAR cxx_supports_sanitizers
  )
  if(NOT cxx_supports_sanitizers)
    message(FATAL_ERROR "Compiler did not accept requested flags.")
  endif()


  try_append_cxx_flags("-Wunused-parameter" TARGET core_interface
    IF_CHECK_PASSED "-Wno-unused-parameter"
  )


  try_append_cxx_flags("-Werror=return-type" TARGET core_interface
    IF_CHECK_FAILED "-Wno-error=return-type"
    SOURCE "#include <cassert>\nint f(){ assert(false); }"
  )


In configuration output, this function prints a string by the following pattern:

  -- Performing Test CXX_SUPPORTS_[flags]
  -- Performing Test CXX_SUPPORTS_[flags] - Success

]=]
function(try_append_cxx_flags flags)
  cmake_parse_arguments(PARSE_ARGV 1
    TACXXF                            # prefix
    ""                                # options
    "SOURCE;TARGET;RESULT_VAR"        # one_value_keywords
    "IF_CHECK_PASSED;IF_CHECK_FAILED" # multi_value_keywords
  )

  string(MAKE_C_IDENTIFIER "${flags}" result)
  string(TOUPPER "${result}" result)
  string(PREPEND result "CXX_SUPPORTS_")

  set(source "int main() { return 0; }")
  if(DEFINED TACXXF_SOURCE AND NOT TACXXF_SOURCE STREQUAL source)
    set(source "${TACXXF_SOURCE}")
    string(SHA256 source_hash "${source}")
    string(SUBSTRING "${source_hash}" 0 4 source_hash_head)
    string(APPEND result "_${source_hash_head}")
  endif()

  # This avoids running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  set(CMAKE_REQUIRED_FLAGS "${flags} ${working_compiler_werror_flag}")
  check_cxx_source_compiles("${source}" ${result})

  if(DEFINED TACXXF_TARGET)
    if(${result})
      if(DEFINED TACXXF_IF_CHECK_PASSED)
        target_compile_options(${TACXXF_TARGET} INTERFACE ${TACXXF_IF_CHECK_PASSED})
      else()
        target_compile_options(${TACXXF_TARGET} INTERFACE ${flags})
      endif()
    elseif(DEFINED TACXXF_IF_CHECK_FAILED)
      target_compile_options(${TACXXF_TARGET} INTERFACE ${TACXXF_IF_CHECK_FAILED})
    endif()
  endif()

  if(DEFINED TACXXF_RESULT_VAR)
    set(${TACXXF_RESULT_VAR} "${${result}}" PARENT_SCOPE)
  endif()
endfunction()

if(MSVC)
  set(working_compiler_werror_flag "/WX /options:strict")
else()
  set(working_compiler_werror_flag "-Werror")
endif()
try_append_cxx_flags("${working_compiler_werror_flag}" RESULT_VAR cxx_supports_werror_flag)
if(NOT cxx_supports_werror_flag)
  set(working_compiler_werror_flag "")
endif()
unset(cxx_supports_werror_flag)
