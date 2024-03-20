# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Check whether evhttp_connection_get_peer expects const char**.
# See https://github.com/libevent/libevent/commit/a18301a2bb160ff7c3ffaf5b7653c39ffe27b385
macro(check_evhttp_connection_get_peer target)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_LIBRARIES ${target})
  check_cxx_source_compiles("
    #include <cstdint>
    #include <event2/http.h>

    int main()
    {
        evhttp_connection* conn = (evhttp_connection*)1;
        const char* host;
        uint16_t port;
        evhttp_connection_get_peer(conn, &host, &port);
    }
    " HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR
  )
  cmake_pop_check_state()
  target_compile_definitions(${target} INTERFACE
    $<$<BOOL:${HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR}>:HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR=1>
  )
endmacro()

function(add_libevent_if_needed)
  # TODO: Not all targets, which will be added in the future,
  #       require libevent. Therefore, a proper check will be
  #       appropriate here.

  set(libevent_minimum_version 2.1.8)

  if(MSVC)
    find_package(Libevent ${libevent_minimum_version} REQUIRED COMPONENTS extra CONFIG)
    check_evhttp_connection_get_peer(libevent::extra)
    add_library(libevent::libevent ALIAS libevent::extra)
    return()
  endif()

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libevent
    REQUIRED IMPORTED_TARGET GLOBAL
    libevent>=${libevent_minimum_version}
  )
  target_link_libraries(PkgConfig::libevent INTERFACE
    # Due to a bug in the libevent build system, the required
    # iphlpapi library is not listed in the libevent.pc file.
    # See upstream:
    #  - https://github.com/libevent/libevent/issues/1110
    #  - https://github.com/libevent/libevent/pull/1111
    # It is not clear which approach is preferable for the upstream
    # maintainers: (1) remove iphlpapi dependency or (2) fix
    # the libevent.pc file generation.
    $<$<PLATFORM_ID:Windows>:iphlpapi>
  )

  check_evhttp_connection_get_peer(PkgConfig::libevent)
  add_library(libevent::libevent ALIAS PkgConfig::libevent)

  if(NOT WIN32)
    pkg_check_modules(libevent_pthreads
      REQUIRED IMPORTED_TARGET GLOBAL
      libevent_pthreads>=${libevent_minimum_version}
    )
  endif()
endfunction()
