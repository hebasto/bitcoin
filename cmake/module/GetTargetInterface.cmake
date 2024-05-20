# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Get target's interface properties recursively.
function(get_target_interface var target property)
  get_target_property(result ${target} INTERFACE_${property})
  if(result)
    string(GENEX_STRIP "${result}" result)
    list(JOIN result " " result)
  else()
    set(result)
  endif()

  get_target_property(dependencies ${target} INTERFACE_LINK_LIBRARIES)
  if(dependencies)
    foreach(dependency IN LISTS dependencies)
      if(TARGET ${dependency})
        get_target_interface(dep_result ${dependency} ${property})
        string(STRIP "${result} ${dep_result}" result)
      endif()
    endforeach()
  endif()

  set(${var} "${result}" PARENT_SCOPE)
endfunction()
