cmake_minimum_required(VERSION 3.25)

project(UniModFramework
    VERSION 0.1.0
    DESCRIPTION "Universal Runtime Modding & Reverse Engineering Framework"
    LANGUAGES C CXX
)

# Standard C++20 requirement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(FetchContent)

# Global options
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Dependency Management: Find or Fetch
# ---------------------------------------------------------------------------

# 1. Spdlog
find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "[UniMod] spdlog not found via find_package, fetching via FetchContent...")
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        GIT_SHALLOW TRUE
    )
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(spdlog)
endif()

# 2. MinHook
find_package(minhook CONFIG QUIET)
if(TARGET minhook::minhook)
    set(MINHOOK_TARGET minhook::minhook)
elseif(TARGET minhook)
    set(MINHOOK_TARGET minhook)
else()
    message(STATUS "[UniMod] minhook not found, fetching source and building directly...")
    FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG v1.3.3
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(minhook)
    if(NOT minhook_POPULATED)
        FetchContent_Populate(minhook)
    endif()

    add_library(minhook STATIC
        ${minhook_SOURCE_DIR}/src/buffer.c
        ${minhook_SOURCE_DIR}/src/hook.c
        ${minhook_SOURCE_DIR}/src/trampoline.c
        ${minhook_SOURCE_DIR}/src/hde/hde64.c
        ${minhook_SOURCE_DIR}/src/hde/hde32.c
    )
    target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)
    add_library(minhook::minhook ALIAS minhook)
    set(MINHOOK_TARGET minhook::minhook)
endif()

# ---------------------------------------------------------------------------
# Core Runtime Library (Static)
# ---------------------------------------------------------------------------
add_library(unimod_core STATIC
    src/core/logger.cpp
    src/core/core_runtime.cpp
)

target_include_directories(unimod_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(unimod_core
    PUBLIC
        spdlog::spdlog
        ${MINHOOK_TARGET}
)

unimod_set_target_warnings(unimod_core)

# ---------------------------------------------------------------------------
# Proxy DLL Target: dinput8.dll
# ---------------------------------------------------------------------------
set(DINPUT8_DEF "${CMAKE_CURRENT_SOURCE_DIR}/proxy/dinput8/dinput8.def")

add_library(dinput8 SHARED
    proxy/dinput8/dllmain.cpp
    proxy/dinput8/dinput8_proxy.cpp
    ${DINPUT8_DEF}
)

target_include_directories(dinput8
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/proxy/dinput8
)

target_link_libraries(dinput8
    PRIVATE
        unimod_core
)

set_target_properties(dinput8 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "dinput8"
)

if(MSVC)
    target_link_options(dinput8 PRIVATE "/DEF:${DINPUT8_DEF}")
else()
    target_link_options(dinput8 PRIVATE "${DINPUT8_DEF}")
endif()

unimod_set_target_warnings(dinput8)

# ---------------------------------------------------------------------------
# Test Targets
# ---------------------------------------------------------------------------
# Phase 1 Test
add_executable(phase1_test
    tests/phase1_test.cpp
)
target_link_libraries(phase1_test PRIVATE unimod_core)
unimod_set_target_warnings(phase1_test)

# Phase 2 Test
add_executable(phase2_test
    tests/phase2_test.cpp
)
add_dependencies(phase2_test dinput8)
unimod_set_target_warnings(phase2_test)
