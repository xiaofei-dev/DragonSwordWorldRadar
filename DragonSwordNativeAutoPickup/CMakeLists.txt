cmake_minimum_required(VERSION 3.25)

project(DragonSwordNativeAutoPickup LANGUAGES CXX)

option(DSNAP_BUILD_TESTS "Build standalone core tests" ON)
option(DSNAP_BUILD_UE4SS "Build the UE4SS native mod adapter" OFF)
set(UE4SS_ROOT "" CACHE PATH "Pinned RE-UE4SS v3.0.1 source tree with initialized submodules")
set(DSNAP_UE4SS_VARIANT "ExperimentalNested" CACHE STRING "UE4SS ABI variant")
set_property(CACHE DSNAP_UE4SS_VARIANT PROPERTY STRINGS ExperimentalNested)

add_library(dsnap_core STATIC
    src/core/candidate_queue.cpp
    src/core/configuration.cpp
    src/core/pe_runtime.cpp
    src/core/pickup_controller.cpp
    src/core/selector_resolver.cpp
)

target_include_directories(dsnap_core PUBLIC include)
target_compile_features(dsnap_core PUBLIC cxx_std_20)
target_compile_definitions(dsnap_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
    target_compile_options(dsnap_core PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
endif()

if(WIN32)
    add_library(dsnap_platform STATIC
        src/platform/async_logger.cpp
        src/platform/windows_fingerprint.cpp
    )
    target_include_directories(dsnap_platform PUBLIC include)
    target_compile_features(dsnap_platform PUBLIC cxx_std_20)
    target_compile_definitions(dsnap_platform PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(dsnap_platform PUBLIC Bcrypt)
    if(MSVC)
        target_compile_options(dsnap_platform PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    endif()
endif()

if(DSNAP_BUILD_TESTS)
    enable_testing()
    add_executable(dsnap_core_tests
        tests/core_tests.cpp
        tests/pe_runtime_tests.cpp
    )
    target_link_libraries(dsnap_core_tests PRIVATE dsnap_core)
    if(WIN32)
        target_link_libraries(dsnap_core_tests PRIVATE dsnap_platform)
    endif()
    target_compile_features(dsnap_core_tests PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(dsnap_core_tests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    endif()
    add_test(NAME dsnap_core_tests COMMAND dsnap_core_tests)
endif()

if(DSNAP_BUILD_UE4SS)
    if(NOT UE4SS_ROOT)
        message(FATAL_ERROR "DSNAP_BUILD_UE4SS requires -DUE4SS_ROOT=<pinned RE-UE4SS v3.0.1 tree>")
    endif()
    if(NOT EXISTS "${UE4SS_ROOT}/UE4SS/include/Mod/CppUserModBase.hpp")
        message(FATAL_ERROR "UE4SS_ROOT does not contain UE4SS/include/Mod/CppUserModBase.hpp")
    endif()
    if(NOT EXISTS "${UE4SS_ROOT}/deps/first/Unreal/include/Unreal/UObjectArray.hpp")
        message(FATAL_ERROR "UE4SS_ROOT is incomplete: initialize the pinned deps/first/Unreal submodule")
    endif()

    if(NOT DSNAP_UE4SS_VARIANT STREQUAL "ExperimentalNested")
        message(FATAL_ERROR "Version 1.3.0 supports ExperimentalNested only: ${DSNAP_UE4SS_VARIANT}")
    endif()
    set(DSNAP_UE4SS_ADAPTER src/ue4ss/main_1_6.cpp)

    # The one supported loader revision uses the required
    # Game__Shipping__Win64 configuration. Historical adapters remain in the
    # source ledger but are not public 1.3.0 build targets.
    add_subdirectory("${UE4SS_ROOT}" "${CMAKE_BINARY_DIR}/ue4ss" EXCLUDE_FROM_ALL)
    add_library(DragonSwordNativeAutoPickup SHARED "${DSNAP_UE4SS_ADAPTER}")
    target_include_directories(DragonSwordNativeAutoPickup PRIVATE include)
    target_link_libraries(DragonSwordNativeAutoPickup PRIVATE dsnap_core dsnap_platform UE4SS Zydis)
    target_compile_features(DragonSwordNativeAutoPickup PRIVATE cxx_std_20)
    target_compile_definitions(DragonSwordNativeAutoPickup PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    if(MSVC)
        target_compile_options(DragonSwordNativeAutoPickup PRIVATE /W4 /WX /wd4251 /wd4324 /permissive- /EHsc /utf-8)
        set_property(SOURCE "${DSNAP_UE4SS_ADAPTER}" APPEND PROPERTY COMPILE_OPTIONS
            /W4 /WX /wd4324 /external:anglebrackets /external:W0 /permissive- /EHsc /utf-8)
    endif()
    set_target_properties(DragonSwordNativeAutoPickup PROPERTIES OUTPUT_NAME "main")
endif()
