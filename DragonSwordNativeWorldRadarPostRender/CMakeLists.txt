cmake_minimum_required(VERSION 3.25)

project(DragonSwordNativeWorldRadarPostRender LANGUAGES CXX)

option(DSNWRPR_BUILD_UE4SS "Build the UE4SS native UMG radar" OFF)
set(DSNWRPR_UE4SS_VARIANT "ExperimentalNested" CACHE STRING "UE4SS ABI variant")
set_property(CACHE DSNWRPR_UE4SS_VARIANT PROPERTY STRINGS ExperimentalNested StableRoot)
set(UE4SS_ROOT "" CACHE PATH "Pinned RE-UE4SS source tree")

add_library(DragonSwordWorldRadarObjectStateCore INTERFACE)
target_include_directories(DragonSwordWorldRadarObjectStateCore INTERFACE include)
target_compile_features(DragonSwordWorldRadarObjectStateCore INTERFACE cxx_std_20)

add_executable(DragonSwordWorldRadarObjectStateTests tests/native_state_tests.cpp)
target_link_libraries(DragonSwordWorldRadarObjectStateTests PRIVATE DragonSwordWorldRadarObjectStateCore)

add_executable(DragonSwordSceneMarkerTests tests/scene_marker_tests.cpp)
target_link_libraries(DragonSwordSceneMarkerTests PRIVATE DragonSwordWorldRadarObjectStateCore)
add_executable(DragonSwordSceneProjectionTests tests/scene_projection_tests.cpp)
target_link_libraries(DragonSwordSceneProjectionTests PRIVATE DragonSwordWorldRadarObjectStateCore)
add_executable(DragonSwordEncounterHeightTests tests/encounter_height_tests.cpp)
target_link_libraries(DragonSwordEncounterHeightTests PRIVATE DragonSwordWorldRadarObjectStateCore)
add_executable(DragonSwordCompactClockLayoutTests tests/compact_clock_layout_tests.cpp)
target_link_libraries(DragonSwordCompactClockLayoutTests PRIVATE DragonSwordWorldRadarObjectStateCore)
add_executable(DragonSwordRadarConfirmationTests tests/radar_confirmation_tests.cpp)
target_link_libraries(DragonSwordRadarConfirmationTests PRIVATE DragonSwordWorldRadarObjectStateCore)
add_executable(DragonSwordHubViewportLayoutTests tests/hub_viewport_layout_tests.cpp)
target_link_libraries(DragonSwordHubViewportLayoutTests PRIVATE DragonSwordWorldRadarObjectStateCore)

add_executable(DragonSwordNativeEventLogTests
    tests/native_event_log_tests.cpp
    src/native/native_event_log.cpp)
target_include_directories(DragonSwordNativeEventLogTests PRIVATE src/native)
target_link_libraries(DragonSwordNativeEventLogTests PRIVATE DragonSwordWorldRadarObjectStateCore)
target_compile_definitions(DragonSwordNativeEventLogTests PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
if(WIN32)
    add_executable(DragonSwordHubEscapeInputTests
        tests/hub_escape_input_tests.cpp
        src/native/hub_escape_input.cpp)
    target_include_directories(DragonSwordHubEscapeInputTests PRIVATE include src/native)
    target_link_libraries(DragonSwordHubEscapeInputTests PRIVATE DragonSwordWorldRadarObjectStateCore user32)
    target_compile_definitions(DragonSwordHubEscapeInputTests PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE DSNWRPR_ESCAPE_INPUT_TEST)
    if(MSVC)
        target_compile_options(DragonSwordHubEscapeInputTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    endif()
endif()
if(MSVC)
    target_compile_options(DragonSwordWorldRadarObjectStateTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordNativeEventLogTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordSceneMarkerTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordSceneProjectionTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordEncounterHeightTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordCompactClockLayoutTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordRadarConfirmationTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    target_compile_options(DragonSwordHubViewportLayoutTests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
endif()

enable_testing()
add_test(NAME native_state_tests COMMAND DragonSwordWorldRadarObjectStateTests)
add_test(NAME native_event_log_tests COMMAND DragonSwordNativeEventLogTests)
add_test(NAME scene_marker_tests COMMAND DragonSwordSceneMarkerTests)
add_test(NAME scene_projection_tests COMMAND DragonSwordSceneProjectionTests)
add_test(NAME encounter_height_tests COMMAND DragonSwordEncounterHeightTests)
add_test(NAME compact_clock_layout_tests COMMAND DragonSwordCompactClockLayoutTests)
add_test(NAME radar_confirmation_tests COMMAND DragonSwordRadarConfirmationTests)
add_test(NAME hub_viewport_layout_tests COMMAND DragonSwordHubViewportLayoutTests)
if(WIN32)
    add_test(NAME hub_escape_input_tests COMMAND DragonSwordHubEscapeInputTests)
endif()

if(DSNWRPR_BUILD_UE4SS)
    if(NOT UE4SS_ROOT OR NOT EXISTS "${UE4SS_ROOT}/UE4SS/include/Mod/CppUserModBase.hpp")
        message(FATAL_ERROR "DSNWRPR_BUILD_UE4SS requires a complete -DUE4SS_ROOT path")
    endif()
    add_subdirectory("${UE4SS_ROOT}" "${CMAKE_BINARY_DIR}/ue4ss" EXCLUDE_FROM_ALL)
    add_library(DragonSwordNativeWorldRadarPostRender SHARED
        src/native/main.cpp
        src/native/native_event_log.cpp
        src/native/compact_umg_renderer.cpp
        src/native/scene_umg_renderer.cpp
        src/native/radar_visibility_hub.cpp
        src/native/hub_escape_input.cpp
        src/native/world_map_umg_renderer.cpp
        src/native/native_save_reconciler.cpp)
    target_link_libraries(DragonSwordNativeWorldRadarPostRender PRIVATE UE4SS DragonSwordWorldRadarObjectStateCore user32)
    if(DSNWRPR_UE4SS_VARIANT STREQUAL "StableRoot")
        target_include_directories(DragonSwordNativeWorldRadarPostRender BEFORE PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/native/stable_compat")
    endif()
    target_compile_features(DragonSwordNativeWorldRadarPostRender PRIVATE cxx_std_20)
    target_compile_definitions(DragonSwordNativeWorldRadarPostRender PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    if(DSNWRPR_UE4SS_VARIANT STREQUAL "StableRoot")
        target_compile_definitions(DragonSwordNativeWorldRadarPostRender PRIVATE DSNWRPR_UE4SS_STABLE_ROOT=1)
    elseif(NOT DSNWRPR_UE4SS_VARIANT STREQUAL "ExperimentalNested")
        message(FATAL_ERROR "Unsupported DSNWRPR_UE4SS_VARIANT: ${DSNWRPR_UE4SS_VARIANT}")
    endif()
    if(MSVC)
        target_compile_options(DragonSwordNativeWorldRadarPostRender PRIVATE /W4 /WX /wd4251 /wd4324 /wd5038 /permissive- /EHsc /utf-8)
        set_property(SOURCE src/native/main.cpp src/native/native_event_log.cpp src/native/compact_umg_renderer.cpp src/native/scene_umg_renderer.cpp src/native/radar_visibility_hub.cpp src/native/hub_escape_input.cpp src/native/world_map_umg_renderer.cpp src/native/native_save_reconciler.cpp APPEND PROPERTY COMPILE_OPTIONS
            /W4 /WX /wd4324 /external:anglebrackets /external:W0 /permissive- /EHsc /utf-8)
    endif()
    set_target_properties(DragonSwordNativeWorldRadarPostRender PROPERTIES OUTPUT_NAME "main")
endif()
