cmake_minimum_required(VERSION 3.13...3.24)

project(ccache-msvc LANGUAGES CXX)

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
