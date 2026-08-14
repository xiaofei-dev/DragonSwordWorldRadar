cmake_minimum_required(VERSION 3.25)

project(DragonSwordWorldRadarObjectState LANGUAGES CXX)

option(DSWROS_BUILD_UE4SS "Build the UE4SS native provider" OFF)
set(UE4SS_ROOT "" CACHE PATH "Pinned RE-UE4SS source tree")

add_library(DragonSwordWorldRadarObjectStateCore INTERFACE)
target_include_directories(DragonSwordWorldRadarObjectStateCore INTERFACE include)
target_compile_features(DragonSwordWorldRadarObjectStateCore INTERFACE cxx_std_20)

add_executable(DragonSwordWorldRadarObjectStateTests tests/native_state_tests.cpp)
target_link_libraries(DragonSwordWorldRadarObjectStateTests PRIVATE DragonSwordWorldRadarObjectStateCore)
if(MSVC)
    target_compile_options(DragonSwordWorldRadarObjectStateTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
endif()

enable_testing()
add_test(NAME native_state_tests COMMAND DragonSwordWorldRadarObjectStateTests)

if(DSWROS_BUILD_UE4SS)
    if(NOT UE4SS_ROOT OR NOT EXISTS "${UE4SS_ROOT}/UE4SS/include/Mod/CppUserModBase.hpp")
        message(FATAL_ERROR "DSWROS_BUILD_UE4SS requires a complete -DUE4SS_ROOT path")
    endif()
    add_subdirectory("${UE4SS_ROOT}" "${CMAKE_BINARY_DIR}/ue4ss" EXCLUDE_FROM_ALL)
    add_library(DragonSwordWorldRadarObjectState SHARED src/native/main.cpp)
    target_link_libraries(DragonSwordWorldRadarObjectState PRIVATE UE4SS DragonSwordWorldRadarObjectStateCore)
    target_compile_features(DragonSwordWorldRadarObjectState PRIVATE cxx_std_20)
    target_compile_definitions(DragonSwordWorldRadarObjectState PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(DragonSwordWorldRadarObjectState PRIVATE /W4 /WX /wd4251 /wd4324 /wd5038 /permissive- /EHsc /utf-8)
        set_property(SOURCE src/native/main.cpp APPEND PROPERTY COMPILE_OPTIONS
            /W4 /WX /wd4324 /external:anglebrackets /external:W0 /permissive- /EHsc /utf-8)
    endif()
    set_target_properties(DragonSwordWorldRadarObjectState PROPERTIES OUTPUT_NAME "main")
endif()
