cmake_minimum_required(VERSION 3.24)

project(nozzle_spout_syphon
    VERSION 0.1.0
    DESCRIPTION "Spout/Syphon bridge scaffold for nozzle texture sharing"
    LANGUAGES C CXX
)

option(NOZZLE_SPOUT_SYPHON_BUILD_TESTS "Build nozzle-spout-syphon tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX AND NOT APPLE)
    message(FATAL_ERROR "nozzle-spout-syphon targets macOS/Syphon and Windows/Spout only; Linux is out of scope")
endif()

if(APPLE)
    enable_language(OBJCXX)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(NOZZLE_INSTALL OFF CACHE BOOL "Disable nozzle install rules" FORCE)
set(NOZZLE_BUILD_TESTS OFF CACHE BOOL "Disable nozzle tests for app builds" FORCE)
set(NOZZLE_BUILD_EXAMPLES OFF CACHE BOOL "Disable nozzle examples for app builds" FORCE)
set(NOZZLE_BUILD_OPENGL ON CACHE BOOL "Enable nozzle OpenGL interop for future Syphon/Spout paths" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nozzle/CMakeLists.txt")
    message(FATAL_ERROR "missing nozzle submodule; run: git submodule update --init --recursive")
endif()
add_subdirectory(nozzle)

set(NOZZLE_SPOUT_SYPHON_APP_SOURCES
    src/app/app_config.cpp
    src/app/status_report.cpp
)

if(APPLE)
    list(APPEND NOZZLE_SPOUT_SYPHON_APP_SOURCES src/platform/macos/syphon_bridge.mm)
elseif(WIN32)
    list(APPEND NOZZLE_SPOUT_SYPHON_APP_SOURCES src/platform/windows/spout_bridge.cpp)
else()
    message(FATAL_ERROR "unsupported platform")
endif()

add_library(nozzle_spout_syphon_app STATIC ${NOZZLE_SPOUT_SYPHON_APP_SOURCES})
target_include_directories(nozzle_spout_syphon_app PUBLIC src)
target_link_libraries(nozzle_spout_syphon_app PUBLIC nozzle)
target_compile_options(nozzle_spout_syphon_app PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
)

target_compile_definitions(nozzle_spout_syphon_app PUBLIC
    NOZZLE_SPOUT_SYPHON_VERSION="${PROJECT_VERSION}"
)

add_executable(nozzle-spout-syphon src/main.cpp)
target_link_libraries(nozzle-spout-syphon PRIVATE nozzle_spout_syphon_app)
target_compile_options(nozzle-spout-syphon PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
)

if(APPLE)
    set_target_properties(nozzle-spout-syphon PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER org.nozzle-io.spout-syphon
        MACOSX_BUNDLE_BUNDLE_NAME "Nozzle Spout Syphon"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
endif()

if(NOZZLE_SPOUT_SYPHON_BUILD_TESTS)
    include(CTest)
    add_executable(nozzle_spout_syphon_tests tests/test_main.cpp)
    target_link_libraries(nozzle_spout_syphon_tests PRIVATE nozzle_spout_syphon_app)
    target_compile_options(nozzle_spout_syphon_tests PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wall -Wextra -Wpedantic -fno-exceptions>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -fno-exceptions>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
    )
    add_test(NAME nozzle_spout_syphon_tests COMMAND nozzle_spout_syphon_tests)
endif()
