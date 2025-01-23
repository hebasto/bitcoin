# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Evaluates config-specific generator expressions in a list.
# Recognizable patterns are:
#  - $<$<CONFIG:[config]>:[value]>
#  - $<$<NOT:$<CONFIG:[config]>>:[value]>
function(evaluate_generator_expressions list config)
  set(input ${${list}})
  set(result)
  foreach(token IN LISTS input)
    if(token MATCHES "\\$<\\$<CONFIG:([^>]+)>:([^>]+)>")
      if(CMAKE_MATCH_1 STREQUAL config)
        list(APPEND result ${CMAKE_MATCH_2})
      endif()
    elseif(token MATCHES "\\$<\\$<NOT:\\$<CONFIG:([^>]+)>>:([^>]+)>")
      if(NOT CMAKE_MATCH_1 STREQUAL config)
        list(APPEND result ${CMAKE_MATCH_2})
      endif()
    else()
      list(APPEND result ${token})
    endif()
  endforeach()
  set(${list} ${result} PARENT_SCOPE)
endfunction()


# Gets target's interface properties recursively.
function(get_target_interface var config target property)
  get_target_property(result ${target} INTERFACE_${property})
  if(result)
    # The CMake FindThreads module uses generator expressions to conditionally apply
    # the `-pthread` flag, avoiding its addition in cases such as CUDA or Swift, which
    # are not applicable to this project. These expressions are effectively equivalent
    # to using the `-pthread` flag directly.
    if(${target} STREQUAL "Threads::Threads" AND ${property} STREQUAL "COMPILE_OPTIONS")
      set(result -pthread)
    else()
      evaluate_generator_expressions(result "${config}")
    endif()
    list(JOIN result " " result)
  else()
    set(result)
  endif()

  get_target_property(dependencies ${target} INTERFACE_LINK_LIBRARIES)
  if(dependencies)
    evaluate_generator_expressions(dependencies "${config}")
    foreach(dependency IN LISTS dependencies)
      if(TARGET ${dependency})
        get_target_interface(dep_result "${config}" ${dependency} ${property})
        string(STRIP "${result} ${dep_result}" result)
      endif()
    endforeach()
  endif()

  set(${var} "${result}" PARENT_SCOPE)
endfunction()
