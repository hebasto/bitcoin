cmake_minimum_required(VERSION 3.13...3.24)

project(ccache-msvc LANGUAGES CXX)

find_program(ccache_shim ccache)

set(MSVC_CCACHE_WRAPPER_CONTENT "\"${ccache_shim}\" \"${CMAKE_CXX_COMPILER}\"")
set(MSVC_CCACHE_WRAPPER_FILENAME wrapped-cl.bat)
file(WRITE ${CMAKE_BINARY_DIR}/${MSVC_CCACHE_WRAPPER_FILENAME} "${MSVC_CCACHE_WRAPPER_CONTENT} %*")

set(CMAKE_VS_GLOBALS
  "CLToolExe=${MSVC_CCACHE_WRAPPER_FILENAME}"
  "CLToolPath=${CMAKE_BINARY_DIR}"
  "TrackFileAccess=false"
  "UseMultiToolTask=true"
  "DebugInformationFormat=OldStyle"
)

string(REGEX REPLACE "/Zi[ \t\r\n]*" "/Z7 " CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REGEX REPLACE "/Zi[ \t\r\n]*" "/Z7 " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

add_executable(main main.cpp)

message("Supported configurations .............. ${CMAKE_CONFIGURATION_TYPES}")
message("Debug configuration:")
message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_DEBUG}")
message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_DEBUG}")
message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
message("Release configuration:")
message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_RELEASE}")
message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_RELEASE}")
message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
