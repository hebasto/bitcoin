# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(try_append_cflag flags_var flag)
  string(MAKE_C_IDENTIFIER "${flag}" result)
  string(TOUPPER "${result}" result)
  set(result "C_SUPPORTS${result}")
  unset(${result})
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  if(NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "-Werror")
  endif()

  include(CheckCCompilerFlag)
  check_c_compiler_flag(${flag} ${result})

  if(${result})
    string(STRIP "${${flags_var}} ${flag}" ${flags_var})
    set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
  endif()
  set(${result} "${${result}}" PARENT_SCOPE)
endfunction()

function(try_append_cxxflag flags_var flag)
  cmake_parse_arguments(PARSE_ARGV 2 TRY_APPEND_CXXFLAG
    "" "SOURCE" "CHECK_PASSED_FLAG;CHECK_FAILED_FLAG"
  )
  string(MAKE_C_IDENTIFIER "${flag}" result)
  string(TOUPPER "${result}" result)
  set(result "CXX_SUPPORTS${result}")
  unset(${result})
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  if(NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "${flag} -Werror")
  endif()

  if(TRY_APPEND_CXXFLAG_SOURCE)
    set(source "${TRY_APPEND_CXXFLAG_SOURCE}")
    unset(${result} CACHE)
  else()
    set(source "int main() { return 0; }")
  endif()

  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${source}" ${result})

  if(${result})
    if(DEFINED TRY_APPEND_CXXFLAG_CHECK_PASSED_FLAG)
      string(STRIP "${${flags_var}} ${TRY_APPEND_CXXFLAG_CHECK_PASSED_FLAG}" ${flags_var})
    else()
      string(STRIP "${${flags_var}} ${flag}" ${flags_var})
    endif()
  elseif(DEFINED TRY_APPEND_CXXFLAG_CHECK_FAILED_FLAG)
    string(STRIP "${${flags_var}} ${TRY_APPEND_CXXFLAG_CHECK_FAILED_FLAG}" ${flags_var})
  endif()
  set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
  set(${result} "${${result}}" PARENT_SCOPE)
  set(try_append_cxxflag_result "${${result}}" PARENT_SCOPE)
endfunction()
