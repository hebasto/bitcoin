# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(add_threads_if_needed)
  # TODO: Not all targets, which will be added in the future,
  #       require Threads. Therefore, a proper check will be
  #       appropriate here.

  if(CMAKE_C_COMPILER_LOADED)
    message(FATAL_ERROR [=[
  To make FindThreads check C++ language features, C language must be
  disabled. This is essential, at least, when cross-compiling for MinGW-w64
  because two different threading models are available.
    ]=] )
  endif()

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set_target_properties(Threads::Threads PROPERTIES IMPORTED_GLOBAL TRUE)
endfunction()
