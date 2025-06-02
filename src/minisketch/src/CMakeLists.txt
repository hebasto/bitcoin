include(GNUInstallDirs)

add_subdirectory(fields)

add_compile_definitions($<$<BOOL:${HAVE_CLZ}>:HAVE_CLZ>)

add_library(minisketch minisketch.cpp)
target_include_directories(minisketch INTERFACE
  $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${PROJECT_IS_TOP_LEVEL}>>:${PROJECT_SOURCE_DIR}/include>>
)
target_link_libraries(minisketch PRIVATE minisketch_field_sources)

add_library(minisketch_verify EXCLUDE_FROM_ALL minisketch.cpp)
target_compile_definitions(minisketch_verify
  PUBLIC
    MINISKETCH_VERIFY
)
target_link_libraries(minisketch_verify PRIVATE minisketch_field_sources)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(minisketch_verify
    PRIVATE
      /wd4702
  )
endif()

if(MINISKETCH_BUILD_TESTS)
  add_executable(test-noverify test.cpp)
  target_link_libraries(test-noverify PRIVATE minisketch)
  add_test(NAME ${PROJECT_NAME}_test_noverify COMMAND test-noverify)

  add_executable(test-verify test.cpp)
  target_link_libraries(test-verify PRIVATE minisketch_verify)
  add_test(NAME ${PROJECT_NAME}_test_verify COMMAND test-verify)
endif()

if(MINISKETCH_BUILD_BENCHMARK)
  add_executable(bench bench.cpp)
  target_link_libraries(bench PRIVATE minisketch)
endif()

if(MINISKETCH_INSTALL)
  install(TARGETS minisketch
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(FILES ${PROJECT_SOURCE_DIR}/include/minisketch.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()
