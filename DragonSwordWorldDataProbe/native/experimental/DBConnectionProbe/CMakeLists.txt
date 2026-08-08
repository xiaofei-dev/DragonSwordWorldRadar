cmake_minimum_required(VERSION 3.25)
project(DragonSwordDBConnectionProbe LANGUAGES CXX)

add_library(DBConnectionProbe SHARED
    src/dllmain.cpp
    src/db_probe.cpp
)

target_include_directories(DBConnectionProbe PRIVATE include)
target_compile_features(DBConnectionProbe PRIVATE cxx_std_20)
target_compile_definitions(DBConnectionProbe PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_compile_options(DBConnectionProbe PRIVATE
    /W4
    /permissive-
    /EHsc
    /utf-8
)
target_link_libraries(DBConnectionProbe PRIVATE Psapi)

set_target_properties(DBConnectionProbe PROPERTIES
    OUTPUT_NAME "DBConnectionProbe"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
