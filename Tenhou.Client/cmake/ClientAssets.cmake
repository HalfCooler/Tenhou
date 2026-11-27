# Generate metadata, embedded resources and copy/install rules from one inventory.
set(asset_generated_directory "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${asset_generated_directory}")
set(TENHOU_ASSET_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" CACHE PATH "External client assets")
set(asset_ids "")
set(asset_entries "")
set(embedded_manifest "")
set(embedded_sources "")
set(asset_keys "")
set(asset_resource_ids "")
# Keep Unicode asset paths out of native makefiles and shell command lines.
# NMake can misdecode them even when the console code page is UTF-8.
set(asset_copy_script "# Generated from assets/catalog.cmake. Do not edit.\n")

macro(tenhou_asset key path storage resource_id requirement legacy)
    if ("${key}" IN_LIST asset_keys)
        message(FATAL_ERROR "Duplicate asset key: ${key}")
    endif ()
    list(APPEND asset_keys "${key}")
    set(asset_required false)
    if ("${requirement}" STREQUAL "REQUIRED")
        set(asset_required true)
    endif ()
    if ("${storage}" STREQUAL "EMBEDDED")
        if ("${resource_id}" IN_LIST asset_resource_ids)
            message(FATAL_ERROR "Duplicate embedded resource ID: ${resource_id}")
        endif ()
        list(APPEND asset_resource_ids "${resource_id}")
        set(asset_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/${path}")
    else ()
        set(asset_source "${TENHOU_ASSET_DIRECTORY}/${path}")
        if (NOT EXISTS "${asset_source}" AND EXISTS "${TENHOU_ASSET_DIRECTORY}/${legacy}")
            set(asset_source "${TENHOU_ASSET_DIRECTORY}/${legacy}")
        endif ()
    endif ()
    if (asset_required AND NOT EXISTS "${asset_source}")
        message(FATAL_ERROR "Missing required asset ${key}: ${asset_source}")
    endif ()
    string(APPEND asset_ids "        ${key},\n")
    string(APPEND asset_entries "        {AssetId::${key}, \"${key}\", \"${path}\", \"${legacy}\", ${resource_id}, ${asset_required}},\n")
    if (EXISTS "${asset_source}")
        if ("${storage}" STREQUAL "EMBEDDED")
            string(APPEND embedded_manifest "${key}|${asset_source}\n")
            list(APPEND embedded_sources "${asset_source}")
        else ()
            get_filename_component(asset_subdirectory "${path}" DIRECTORY)
            string(APPEND asset_copy_script
                "file(MAKE_DIRECTORY [==[${TENHOU_OUTPUT_DIRECTORY}/resources/${asset_subdirectory}]==])\n"
                "file(COPY_FILE [==[${asset_source}]==] [==[${TENHOU_OUTPUT_DIRECTORY}/resources/${path}]==] ONLY_IF_DIFFERENT)\n")
            install(FILES "${asset_source}" DESTINATION "client/resources/${asset_subdirectory}")
        endif ()
    endif ()
endmacro()

set(sprite_entries "")
set(sprite_override_paths "")
set(present_sprite_ids "")
macro(tenhou_sprite id)
    tenhou_asset(Sprite${id} "images/sprites/${id}.png" FILE 0 OPTIONAL -)
    string(APPEND sprite_entries "        {${id}, AssetId::Sprite${id}},\n")
    list(APPEND sprite_override_paths "${TENHOU_ASSET_DIRECTORY}/images/sprites/${id}.png")
    if (EXISTS "${TENHOU_ASSET_DIRECTORY}/images/sprites/${id}.png")
        list(APPEND present_sprite_ids "${id}")
    endif ()
endmacro()
include("${CMAKE_CURRENT_SOURCE_DIR}/assets/catalog.cmake")
# Watch only optional runtime artwork for additions and removals.
file(GLOB authored_sprite_files CONFIGURE_DEPENDS ${sprite_override_paths})

# Clear files managed by the earlier manifest from package outputs on upgrade.
set(sprite_cleanup_script "${asset_generated_directory}/remove_retired_sprites.cmake")
file(CONFIGURE OUTPUT "${sprite_cleanup_script}" CONTENT [=[
cmake_minimum_required(VERSION 3.24)
if(NOT IS_ABSOLUTE "${TENHOU_STAGED_RESOURCES}")
    message(FATAL_ERROR "Expected an absolute staged resource directory")
endif()
get_filename_component(sprite_directory "${TENHOU_STAGED_RESOURCES}/images/sprites" ABSOLUTE)
if(NOT IS_DIRECTORY "${sprite_directory}")
    return()
endif()
file(REAL_PATH "${sprite_directory}" resolved_directory)
if(WIN32)
    string(TOLOWER "${sprite_directory}" expected_directory)
    string(TOLOWER "${resolved_directory}" resolved_directory)
else()
    set(expected_directory "${sprite_directory}")
endif()
# A linked staging directory may point to an authored resource tree.
if(NOT resolved_directory STREQUAL expected_directory)
    return()
endif()
set(present_sprites @present_sprite_ids@)
foreach(id RANGE 401 705)
    if(id IN_LIST present_sprites OR id EQUAL 593)
        continue()
    endif()
    set(path "${sprite_directory}/${id}.png")
    if(IS_SYMLINK "${path}" OR (EXISTS "${path}" AND NOT IS_DIRECTORY "${path}"))
        file(REMOVE "${path}")
    endif()
endforeach()
if(IS_DIRECTORY "${sprite_directory}/hd")
    file(REMOVE_RECURSE "${sprite_directory}/hd")
endif()
]=] @ONLY)
string(APPEND asset_copy_script
    "set(TENHOU_STAGED_RESOURCES [==[${TENHOU_OUTPUT_DIRECTORY}/resources]==])\n"
    "include([==[${sprite_cleanup_script}]==])\n")
install(CODE "get_filename_component(TENHOU_STAGED_RESOURCES \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/client/resources\" ABSOLUTE)
include([==[${sprite_cleanup_script}]==])")

file(WRITE "${asset_generated_directory}/stage_assets.cmake.in" "${asset_copy_script}")
configure_file("${asset_generated_directory}/stage_assets.cmake.in" "${asset_generated_directory}/stage_assets.cmake" COPYONLY)
file(WRITE "${asset_generated_directory}/sprite_assets.hpp.in"
    "// Generated from assets/catalog.cmake.\n#pragma once\n#include \"asset_catalog.hpp\"\nnamespace tenhou {\nstruct SpriteAsset { unsigned id; AssetId asset; };\ninline constexpr SpriteAsset kSpriteAssets[]{\n${sprite_entries}};\n}\n")
configure_file("${asset_generated_directory}/sprite_assets.hpp.in" "${asset_generated_directory}/sprite_assets.hpp" COPYONLY)
set(asset_header "// Generated from assets/catalog.cmake. Do not edit.\n#pragma once\n#include <cstddef>\nnamespace tenhou {\n    enum class AssetId {\n${asset_ids}        Count\n    };\n    struct AssetInfo {\n        AssetId id;\n        const char *name;\n        const char *path;\n        const char *legacy_filename;\n        unsigned embedded_id;\n        bool required;\n    };\n    inline constexpr AssetInfo kClientAssets[]{\n${asset_entries}    };\n    static_assert(sizeof(kClientAssets) / sizeof(kClientAssets[0]) == static_cast<std::size_t>(AssetId::Count));\n}\n")
# configure_file preserves timestamps when catalog contents have not changed.
file(WRITE "${asset_generated_directory}/asset_catalog.hpp.in" "${asset_header}")
configure_file("${asset_generated_directory}/asset_catalog.hpp.in" "${asset_generated_directory}/asset_catalog.hpp" COPYONLY)
file(WRITE "${asset_generated_directory}/embedded_assets.hpp.in"
    "// Generated from assets/catalog.cmake. Do not edit.\n#pragma once\n#include \"resource_manager.hpp\"\nnamespace tenhou {\n    EmbeddedAsset FindEmbeddedAsset(AssetId id);\n}\n")
configure_file("${asset_generated_directory}/embedded_assets.hpp.in" "${asset_generated_directory}/embedded_assets.hpp" COPYONLY)
file(WRITE "${asset_generated_directory}/embedded_assets.manifest.in" "${embedded_manifest}")
configure_file("${asset_generated_directory}/embedded_assets.manifest.in" "${asset_generated_directory}/embedded_assets.manifest" COPYONLY)
add_custom_command(
    OUTPUT "${asset_generated_directory}/embedded_assets.cpp"
    COMMAND "${CMAKE_COMMAND}"
            "-DMANIFEST_FILE=${asset_generated_directory}/embedded_assets.manifest"
            "-DOUTPUT_CPP=${asset_generated_directory}/embedded_assets.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedAssets.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedAssets.cmake"
            "${asset_generated_directory}/embedded_assets.manifest"
            ${embedded_sources}
    COMMENT "Generating portable C++ static embedded assets"
    VERBATIM)
