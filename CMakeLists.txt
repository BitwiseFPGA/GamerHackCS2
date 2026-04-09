cmake_minimum_required(VERSION 3.20)
project(GamerHackCS2 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# x64 only
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "GamerHackCS2 requires a 64-bit (x64) build. Reconfigure with -A x64.")
endif()

# ============================================================================
# FetchContent — automatic dependency downloads
# ============================================================================

include(FetchContent)

# --- ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# --- MinHook ---
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minhook)

# --- nlohmann/json ---
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# --- FreeType ---
set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(freetype)

# ============================================================================
# ImGui sources (compiled as part of this project)
# ============================================================================

set(SOURCES_IMGUI
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    # FreeType integration
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
)

# ============================================================================
# Project source files
# ============================================================================

set(SOURCES_CORE
    src/core/core.cpp
    src/core/config.cpp
    src/core/hooks.cpp
    src/core/interfaces.cpp
    src/core/menu.cpp
    src/core/schema.cpp
)

set(SOURCES_SDK
    src/sdk/entity.cpp
    src/sdk/functionlist.cpp
    src/sdk/econ/itemdefinition.cpp
    src/sdk/gameutils/cl_players.cpp
    src/sdk/gameutils/cl_weapons.cpp
    src/sdk/gameutils/cl_entity_listener.cpp
)

set(SOURCES_FEATURES
    src/features/features.cpp
    src/features/bypass/bypass.cpp
    src/features/inventory/inventory.cpp
    src/features/legitbot/legitbot.cpp
    src/features/misc/misc.cpp
    src/features/visuals/visuals.cpp
)

set(SOURCES_UTILITIES
    src/utilities/bones.cpp
    src/utilities/input.cpp
    src/utilities/log.cpp
    src/utilities/math.cpp
    src/utilities/memory.cpp
    src/utilities/notify.cpp
    src/utilities/render.cpp
    src/utilities/trace.cpp
)

set(SOURCES_ENTRY
    src/dllmain.cpp
)

# ============================================================================
# DLL target
# ============================================================================

add_library(${PROJECT_NAME} SHARED
    ${SOURCES_ENTRY}
    ${SOURCES_CORE}
    ${SOURCES_SDK}
    ${SOURCES_FEATURES}
    ${SOURCES_UTILITIES}
    ${SOURCES_IMGUI}
)

# ============================================================================
# Include directories
# ============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # ImGui headers
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/freetype
    # FreeType headers (for imgui_freetype)
    ${freetype_SOURCE_DIR}/include
)

# ============================================================================
# Link libraries
# ============================================================================

target_link_libraries(${PROJECT_NAME} PRIVATE
    # DirectX
    d3d11
    dxgi
    dxguid
    # Dependencies
    minhook
    freetype
    nlohmann_json::nlohmann_json
)

# ============================================================================
# Preprocessor definitions
# ============================================================================

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
    # Enable ImGui FreeType font rendering
    IMGUI_ENABLE_FREETYPE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# ============================================================================
# Compiler options (MSVC)
# ============================================================================

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3           # Warning level 3
        /MP           # Multi-process compilation
        /EHsc         # C++ exception handling
        /permissive-  # Standards conformance
        /Zc:preprocessor  # Conformant preprocessor
    )

    # Release optimizations
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2>     # Full optimization
        $<$<CONFIG:Release>:/Oi>     # Intrinsic functions
        $<$<CONFIG:Release>:/GS->    # Disable buffer security checks
        $<$<CONFIG:Release>:/Gy>     # Function-level linking
        # Note: /GL (LTCG) removed due to MSVC ICE with ImGui sources
    )

    # Release link-time optimizations
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>    # Remove unreferenced functions
        $<$<CONFIG:Release>:/OPT:ICF>    # Identical COMDAT folding
    )
endif()

# DLL entry point - use CRT startup (it calls DllMain for us)
# Do NOT use /ENTRY:DllMain as it bypasses CRT initialization
# which breaks std::string, JSON, streams, etc.

# ============================================================================
# Output configuration
# ============================================================================

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "GamerHackCS2"
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ============================================================================
# Source grouping for IDE (Visual Studio filters)
# ============================================================================

source_group("Entry"       FILES ${SOURCES_ENTRY})
source_group("Core"        FILES ${SOURCES_CORE})
source_group("SDK"         FILES ${SOURCES_SDK})
source_group("Features"    FILES ${SOURCES_FEATURES})
source_group("Utilities"   FILES ${SOURCES_UTILITIES})
source_group("ImGui"       FILES ${SOURCES_IMGUI})
