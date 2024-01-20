cmake_minimum_required(VERSION 3.16...3.28)

project(TestSSP CXX)

add_executable(test test.cpp)
target_compile_options(test PRIVATE -fstack-protector-all)
