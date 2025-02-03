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
    elseif(token MATCHES "\\$<\\$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:[^>]+>")
      # do nothing
    elseif(token MATCHES "\\$<\\$<AND:\\$<NOT:\\$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>>,\\$<NOT:\\$<COMPILE_LANGUAGE:Swift>>>:([^>]+)>")
      list(APPEND result ${CMAKE_MATCH_1})
    else()
      list(APPEND result ${token})
    endif()
  endforeach()
  set(${list} ${result} PARENT_SCOPE)
endfunction()


# Commit d7963aa9ee38bf26ab31433f2e7bfaff7ddf6c57 - OK, v3.26.0
# set(flags
#   "$<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:-Xcompiler -pthread>"
#   "$<$<AND:$<NOT:$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>>,$<NOT:$<COMPILE_LANGUAGE:Swift>>>:-pthread>"
# )

# Commit 80d37167fed1178872d28cbcbf57c7a3660bf244
set(flags
  "$<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:-Xcompiler -pthread>"
  "$<$<NOT:$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>>:-pthread>"
)


evaluate_generator_expressions(flags "")

message(${flags})
