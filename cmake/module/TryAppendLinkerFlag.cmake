# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(try_append_linker_flag flags_var flag)
  cmake_parse_arguments(PARSE_ARGV 2 TRY_APPEND_LINKER_FLAG
    "" "SOURCE" "CHECK_PASSED_FLAG;CHECK_FAILED_FLAG"
  )
  string(MAKE_C_IDENTIFIER "${flag}" result)
  string(TOUPPER "${result}" result)
  set(result "LINKER_SUPPORTS${result}")
  unset(${result})
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)

  if(CMAKE_VERSION VERSION_LESS 3.14)
    set(linker_flags_var CMAKE_REQUIRED_LIBRARIES)
  else()
    set(linker_flags_var CMAKE_REQUIRED_LINK_OPTIONS)
  endif()

  set(${linker_flags_var} ${flag})

  if(MSVC)
    set(${linker_flags_var} ${flag} /WX)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(${linker_flags_var} ${flag} -Wl,-fatal_warnings)
  else()
    set(${linker_flags_var} ${flag} -Wl,--fatal-warnings)
  endif()

  if(TRY_APPEND_LINKER_FLAG_SOURCE)
    set(source "${TRY_APPEND_LINKER_FLAG_SOURCE}")
    unset(${result} CACHE)
  else()
    set(source "int main() { return 0; }")
  endif()

  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${source}" ${result})

  if(${result})
    if(DEFINED TRY_APPEND_LINKER_FLAG_CHECK_PASSED_FLAG)
      string(STRIP "${${flags_var}} ${TRY_APPEND_LINKER_FLAG_CHECK_PASSED_FLAG}" ${flags_var})
    else()
      string(STRIP "${${flags_var}} ${flag}" ${flags_var})
    endif()
  elseif(DEFINED TRY_APPEND_LINKER_FLAG_CHECK_FAILED_FLAG)
    string(STRIP "${${flags_var}} ${TRY_APPEND_LINKER_FLAG_CHECK_FAILED_FLAG}" ${flags_var})
  endif()

  set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
  set(${result} "${${result}}" PARENT_SCOPE)
  set(try_append_linker_flag_result "${${result}}" PARENT_SCOPE)
endfunction()
