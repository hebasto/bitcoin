set(CMAKE_AUTOMOC_MOC_OPTIONS "-p${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(test_bitcoin-qt test_main.cpp)
target_link_libraries(test_bitcoin-qt PRIVATE Qt6::Test)
target_link_options(test_bitcoin-qt PRIVATE -static)
