add_library(minisketch_field_sources INTERFACE)
target_sources(minisketch_field_sources
  INTERFACE
    generic_1byte.cpp
    generic_2bytes.cpp
    generic_3bytes.cpp
    generic_4bytes.cpp
    generic_5bytes.cpp
    generic_6bytes.cpp
    generic_7bytes.cpp
    generic_8bytes.cpp
)

if(HAVE_CLMUL)
  set(clmul_sources
    clmul_1byte.cpp
    clmul_2bytes.cpp
    clmul_3bytes.cpp
    clmul_4bytes.cpp
    clmul_5bytes.cpp
    clmul_6bytes.cpp
    clmul_7bytes.cpp
    clmul_8bytes.cpp
  )
  target_sources(minisketch_field_sources INTERFACE ${clmul_sources})
  set_property(SOURCE ${clmul_sources}
    DIRECTORY ${PROJECT_SOURCE_DIR}/src
    PROPERTY COMPILE_OPTIONS ${CLMUL_CXXFLAGS}
  )
  target_compile_definitions(minisketch_field_sources INTERFACE HAVE_CLMUL)
endif()
