# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)

#[=[
Usage example:

  try_append_linker_flag(core "-Wl,--major-subsystem-version,6")


In configuration output, this function prints a string by the following pattern:

  -- Performing Test LINKER_SUPPORTS_[flag]
  -- Performing Test LINKER_SUPPORTS_[flag] - Success

]=]
function(try_append_linker_flag target flag)
  cmake_parse_arguments(PARSE_ARGV 2
    TALF                              # prefix
    ""                                # options
    "SOURCE;RESULT_VAR"               # one_value_keywords
    "IF_CHECK_PASSED;IF_CHECK_FAILED" # multi_value_keywords
  )

  string(MAKE_C_IDENTIFIER "${flag}" result)
  string(TOUPPER "${result}" result)
  set(result "LINKER_SUPPORTS_${result}")

  # Every subsequent check_cxx_source_compiles((<code> <resultVar>) run will re-use
  # the cached result rather than performing the check again, even if the <code> changes.
  # Removing the cached result in order to force the check to be re-evaluated.
  unset(${result} CACHE)

  if(NOT DEFINED TALF_SOURCE)
    set(TALF_SOURCE "int main() { return 0; }")
  endif()

  # This forces running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
  
  get_target_property(working_linker_werror_flag internal_werror INTERFACE_LINK_OPTIONS)
  if(NOT working_linker_werror_flag)
    set(working_linker_werror_flag)
  endif()
  if(CMAKE_VERSION VERSION_LESS 3.14)
    set(CMAKE_REQUIRED_LIBRARIES ${flag} ${working_linker_werror_flag})
  else()
    set(CMAKE_REQUIRED_LINK_OPTIONS ${flag} ${working_linker_werror_flag})
  endif()

  check_cxx_source_compiles("${TALF_SOURCE}" ${result})

  if(${result})
    if(DEFINED TALF_IF_CHECK_PASSED)
      target_link_options(${target} INTERFACE ${TALF_IF_CHECK_PASSED})
    else()
      target_link_options(${target} INTERFACE ${flag})
    endif()
  elseif(DEFINED TALF_IF_CHECK_FAILED)
    target_link_options(${target} INTERFACE ${TALF_IF_CHECK_FAILED})
  endif()

  set(${TALF_RESULT_VAR} "${${result}}" PARENT_SCOPE)
endfunction()

if(NOT TARGET internal_werror)
  add_library(internal_werror INTERFACE)
endif()
if(MSVC)
  set(warning_as_error_flag /WX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(warning_as_error_flag -Wl,-fatal_warnings)
else()
  set(warning_as_error_flag -Wl,--fatal-warnings)
endif()
try_append_linker_flag(internal_werror "${warning_as_error_flag}")
unset(warning_as_error_flag)
