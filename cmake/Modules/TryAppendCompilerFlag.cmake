# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(CheckCCompilerFlag)
function(try_append_cflag flags_var flag)
  string(MAKE_C_IDENTIFIER ${flag} result)
  string(TOUPPER ${result} result)
  set(result "C_SUPPORTS${result}")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  check_c_compiler_flag(${flag} ${result})
  if(${result})
    string(STRIP "${${flags_var}} ${flag}" ${flags_var})
    set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
  endif()
endfunction()

include(CheckCXXCompilerFlag)
function(try_append_cxxflag flags_var flag)
  string(MAKE_C_IDENTIFIER ${flag} result)
  string(TOUPPER ${result} result)
  set(result "CXX_SUPPORTS${result}")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  check_cxx_compiler_flag(${flag} ${result})
  if(${result})
    string(STRIP "${${flags_var}} ${flag}" ${flags_var})
    set(${flags_var} "${${flags_var}}" PARENT_SCOPE)
  endif()
endfunction()
