cmake_minimum_required(VERSION 3.13...3.24)

project(ccache-msvc LANGUAGES CXX)

find_program(ccache_shim ccache)
cmake_path(GET ccache_shim PARENT_PATH chocolatey_dir)
cmake_path(APPEND chocolatey_dir "../lib/ccache/tools/ccache-4.8-windows-x86_64/ccache.exe" OUTPUT_VARIABLE ccache_exe)
message("ccache_exe is ${ccache_exe}")


file(COPY_FILE
  ${ccache_exe} ${CMAKE_BINARY_DIR}/cl.exe
  ONLY_IF_DIFFERENT
)

set(CMAKE_VS_GLOBALS
  "CLToolExe=cl.exe"
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
