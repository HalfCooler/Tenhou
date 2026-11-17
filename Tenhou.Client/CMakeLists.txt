cmake_minimum_required(VERSION 3.24)
project(Tenhou.Client VERSION 0.27.0 LANGUAGES C CXX RC)
if (MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_UTF8_SOURCE)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif ()
set(TENHOU_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Native runtime output directory")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TENHOU_OUTPUT_DIRECTORY}")
if (NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "Windows and MSVC are required.")
endif ()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(TENHOU_TEST_STATIC_OBJECT_IMAGES OFF CACHE BOOL "Static Object image traversal is disabled" FORCE)
include(cmake/ClientAssets.cmake)
include(cmake/TextCatalog.cmake)
tenhou_generate_text_catalog("${CMAKE_CURRENT_SOURCE_DIR}/assets/text/catalog.json"
    "${asset_generated_directory}/text_catalog.hpp")
include(cmake/TextRendering.cmake)
tenhou_generate_text_render_profile("${CMAKE_CURRENT_SOURCE_DIR}/assets/text/rendering.json"
    "${asset_generated_directory}/text_render_profile.hpp")
add_library(zlib_inflate STATIC vendor/zlib/adler32.c vendor/zlib/crc32.c vendor/zlib/inffast.c
        vendor/zlib/inflate.c vendor/zlib/inftrees.c vendor/zlib/zutil.c vendor/zlib/uncompr.c)
target_include_directories(zlib_inflate PUBLIC vendor/zlib)
target_compile_definitions(zlib_inflate PRIVATE _CRT_SECURE_NO_WARNINGS)
add_library(tenhou_resources STATIC src/resource_pack.cpp src/object_pack.cpp src/resource_manager.cpp)
target_include_directories(tenhou_resources PUBLIC src "${asset_generated_directory}")
target_compile_definitions(tenhou_resources PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(tenhou_resources PRIVATE /W4 /WX /utf-8 /EHsc)
target_link_libraries(tenhou_resources PUBLIC zlib_inflate bcrypt)
add_library(tenhou_text STATIC src/text_renderer.cpp src/final_scene.cpp)
target_link_libraries(tenhou_text PUBLIC tenhou_resources gdiplus gdi32 usp10)
target_compile_options(tenhou_text PRIVATE /W4 /WX /utf-8 /EHsc)
add_library(tenhou_replay STATIC src/replay.cpp src/settlement.cpp)
target_include_directories(tenhou_replay PUBLIC src "${asset_generated_directory}")
target_compile_definitions(tenhou_replay PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(tenhou_replay PRIVATE /W4 /WX /utf-8 /EHsc)
target_link_libraries(tenhou_replay PUBLIC zlib_inflate)
add_library(tenhou_table STATIC src/table_scene.cpp src/variant_artwork.cpp src/replay_effects.cpp src/replay_audio.cpp src/original_display.cpp
        src/wall_scene.cpp src/action_selection_scene.cpp src/settlement_scene.cpp src/client_preferences.cpp src/tile_shadow.cpp)
target_include_directories(tenhou_table PUBLIC src)
target_compile_options(tenhou_table PRIVATE /W4 /WX /utf-8 /EHsc)
target_link_libraries(tenhou_table PUBLIC tenhou_resources tenhou_replay tenhou_text winmm msacm32 gdiplus)
add_library(tenhou_network STATIC src/network_protocol.cpp src/network_client.cpp src/online_game.cpp src/online_actions.cpp src/replay_archive.cpp src/variant_state.cpp)
target_include_directories(tenhou_network PUBLIC src)
target_compile_definitions(tenhou_network PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(tenhou_network PRIVATE /W4 /WX /utf-8 /EHsc)
target_link_libraries(tenhou_network PUBLIC tenhou_replay ws2_32 user32)
set(tenhou_lobby_sources src/background.cpp src/lobby_view.cpp src/matchmaking.cpp src/matchmaking_view.cpp src/online_table.cpp src/settlement_view.cpp
        src/match_intro_view.cpp src/action_selection_view.cpp src/settings_view.cpp src/room_settings_view.cpp src/variant_table_view.cpp
)
add_executable(tenhou_rebuild WIN32 src/main.cpp src/application.cpp src/welcome_view.cpp src/table_view.cpp ${tenhou_lobby_sources}
        "${asset_generated_directory}/client_assets.rc")
target_compile_options(tenhou_rebuild PRIVATE /W4 /WX /utf-8 /EHsc)
target_link_libraries(tenhou_rebuild PRIVATE tenhou_table tenhou_network gdiplus winmm comdlg32 shell32 comctl32 ole32)
# Runs even when only an asset changed, without relinking the executable.
add_custom_target(tenhou_assets
        COMMAND "${CMAKE_COMMAND}" -P "${asset_generated_directory}/stage_assets.cmake"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${TENHOU_OUTPUT_DIRECTORY}/licenses"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/vendor/zlib/LICENSE"
            "${TENHOU_OUTPUT_DIRECTORY}/licenses/zlib-LICENSE.txt"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets/sources/mahjong_graphic/LICENSE"
            "${TENHOU_OUTPUT_DIRECTORY}/licenses/mahjong_graphic-LICENSE.txt"
        VERBATIM)
add_dependencies(tenhou_rebuild tenhou_assets)
install(TARGETS tenhou_rebuild RUNTIME DESTINATION client)
install(FILES vendor/zlib/LICENSE DESTINATION client/licenses RENAME zlib-LICENSE.txt)
install(FILES assets/sources/mahjong_graphic/LICENSE DESTINATION client/licenses RENAME mahjong_graphic-LICENSE.txt)

# Test source ownership stays with the corresponding test project.
option(TENHOU_BUILD_CLIENT_TESTS "Build the client-specific checks" OFF)
option(TENHOU_BUILD_RULE_ORACLE "Build the frozen server rule comparison helper" OFF)
if (TENHOU_BUILD_CLIENT_TESTS)
    include(../Tenhou.Client.Tests/Tests.cmake)
endif ()
if (TENHOU_BUILD_RULE_ORACLE)
    include(../Tenhou.Server.Tests/NativeReference.cmake)
endif ()
