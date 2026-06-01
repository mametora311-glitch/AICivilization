cmake_minimum_required(VERSION 3.20)

project(AICivilization
        VERSION 0.1.0
        LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Language / compiler settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/utf-8 /W3 /permissive- /EHsc)
endif()

# ---------------------------------------------------------------------------
# Dependencies
#
# raylib is built from source via FetchContent. raygui, nlohmann/json and
# cpp-httplib are header-only and are vendored as single headers into external/
# at configure time (downloaded once, then cached -- the built app runs fully
# offline). raylib/raygui provide the window, GUI, rendering and audio;
# nlohmann/json covers settings/save files; cpp-httplib is used only for the
# permitted local VOICEVOX endpoint.
# ---------------------------------------------------------------------------
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/external" CACHE PATH "" FORCE)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(raylib_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# --- Vendored single-header dependencies -----------------------------------
function(aic_vendor_header url dest)
    if(NOT EXISTS "${dest}")
        get_filename_component(_dest_dir "${dest}" DIRECTORY)
        file(MAKE_DIRECTORY "${_dest_dir}")
        message(STATUS "Vendoring header: ${dest}")
        file(DOWNLOAD "${url}" "${dest}" STATUS _dl_status SHOW_PROGRESS)
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            message(FATAL_ERROR "Failed to download ${url}: ${_dl_msg}")
        endif()
    endif()
endfunction()

aic_vendor_header(
    "https://raw.githubusercontent.com/raysan5/raygui/4.0/src/raygui.h"
    "${CMAKE_SOURCE_DIR}/external/raygui/raygui.h")

aic_vendor_header(
    "https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp"
    "${CMAKE_SOURCE_DIR}/external/nlohmann/json.hpp")

aic_vendor_header(
    "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.19.0/httplib.h"
    "${CMAKE_SOURCE_DIR}/external/cpp-httplib/httplib.h")

# ---------------------------------------------------------------------------
# Core library (everything except main.cpp) so both the app and the unit tests
# can link the same code.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE AIC_CORE_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM AIC_CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

add_library(aic_core STATIC ${AIC_CORE_SOURCES})
target_include_directories(aic_core PUBLIC
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/external"             # <nlohmann/json.hpp>
    "${CMAKE_SOURCE_DIR}/external/raygui"      # "raygui.h"
    "${CMAKE_SOURCE_DIR}/external/cpp-httplib" # "httplib.h"
)
target_compile_features(aic_core PUBLIC cxx_std_20)
# raygui.h (vendored, header-only) uses classic CRT calls (sprintf/fopen/...).
target_compile_definitions(aic_core PUBLIC _CRT_SECURE_NO_WARNINGS)
target_link_libraries(aic_core PUBLIC raylib)
if(WIN32)
    # winmm: raylib audio/timers. ws2_32: cpp-httplib sockets.
    # shell32/user32: Shell_NotifyIconW notification fallback.
    target_link_libraries(aic_core PUBLIC winmm ws2_32 shell32 user32)
endif()

# ---------------------------------------------------------------------------
# Application executable
# ---------------------------------------------------------------------------
add_executable(AICivilization "${CMAKE_SOURCE_DIR}/src/main.cpp")
target_link_libraries(AICivilization PRIVATE aic_core)
set_target_properties(AICivilization PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# ---------------------------------------------------------------------------
# Unit tests (tests/*.cpp -> one executable each, linked against aic_core)
# ---------------------------------------------------------------------------
file(GLOB AIC_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
if(AIC_TEST_SOURCES)
    enable_testing()
    foreach(_test_src ${AIC_TEST_SOURCES})
        get_filename_component(_test_name "${_test_src}" NAME_WE)
        add_executable(${_test_name} "${_test_src}")
        target_link_libraries(${_test_name} PRIVATE aic_core)
        set_target_properties(${_test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
        add_test(NAME ${_test_name} COMMAND ${_test_name})
    endforeach()
endif()
