# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

function(check_linker_supports_pie warnings)
  # This forces running a linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)

  # CMAKE_CXX_COMPILE_OPTIONS_PIE is a list, whereas CMAKE_REQUIRED_FLAGS
  # must be a string. Therefore, a proper conversion is required.
  list(JOIN CMAKE_CXX_COMPILE_OPTIONS_PIE " " flags_as_string)

  # Workaround for a bug in the check_pie_supported() function.
  # See: https://gitlab.kitware.com/cmake/cmake/-/issues/26463.
  set(CMAKE_REQUIRED_FLAGS "${flags_as_string}")

  include(CheckPIESupported)
  check_pie_supported(OUTPUT_VARIABLE output LANGUAGES CXX)
  if(CMAKE_CXX_LINK_PIE_SUPPORTED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
  elseif(NOT WIN32)
    # The warning is superfluous for Windows.
    message(WARNING "PIE is not supported at link time: ${output}")
    list(APPEND ${warnings} "Position independent code disabled.")
  endif()
endfunction()
