cmake_minimum_required(VERSION 3.24)

project(nozzle_spout_syphon
    VERSION 0.1.0
    DESCRIPTION "Spout/Syphon bridge scaffold for nozzle texture sharing"
    LANGUAGES C CXX
)

option(NOZZLE_SPOUT_SYPHON_BUILD_TESTS "Build nozzle-spout-syphon tests" ON)
set(SYPHON_FRAMEWORK_DIR "" CACHE PATH "Path to a built Syphon.framework for macOS runtime bridge")
set(SPOUT2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/Spout2" CACHE PATH "Path to the pinned Spout2 source tree")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX AND NOT APPLE)
    message(FATAL_ERROR "nozzle-spout-syphon targets macOS/Syphon and Windows/Spout only; Linux is out of scope")
endif()

if(APPLE)
    enable_language(OBJCXX)
    if(SYPHON_FRAMEWORK_DIR STREQUAL "")
        set(SYPHON_FRAMEWORK_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/build/syphon-derived/Build/Products/Release/Syphon.framework"
            CACHE PATH "Path to a built Syphon.framework for macOS runtime bridge" FORCE)
    endif()
    if(NOT EXISTS "${SYPHON_FRAMEWORK_DIR}/Syphon")
        message(FATAL_ERROR
            "missing Syphon.framework; build it first with: "
            "xcodebuild -project deps/Syphon-Framework/Syphon.xcodeproj "
            "-scheme Syphon -configuration Release -derivedDataPath build/syphon-derived "
            "CODE_SIGNING_ALLOWED=NO MACOSX_DEPLOYMENT_TARGET=12.0 "
            "ARCHS='arm64 x86_64' ONLY_ACTIVE_ARCH=NO build")
    endif()
endif()

if(WIN32)
    if(NOT EXISTS "${SPOUT2_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "missing Spout2 submodule; run: git submodule update --init --recursive")
    endif()
    set(SPOUT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/spout2" CACHE PATH "Embedded Spout2 output directory" FORCE)
    set(SKIP_INSTALL_ALL ON CACHE BOOL "Disable Spout2 install rules" FORCE)
    add_subdirectory("${SPOUT2_SOURCE_DIR}/SPOUTSDK/SpoutGL" deps/Spout2-SpoutGL-build)
    add_subdirectory("${SPOUT2_SOURCE_DIR}/SPOUTSDK/SpoutDirectX/SpoutDX" deps/Spout2-SpoutDX-build)
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
    src/app/source_selection.cpp
    src/app/status_report.cpp
)

if(APPLE)
    list(APPEND NOZZLE_SPOUT_SYPHON_APP_SOURCES src/platform/macos/syphon_bridge.mm)
    set(NOZZLE_BRIDGE_EXECUTABLE_NAME nozzle-syphon)
elseif(WIN32)
    list(APPEND NOZZLE_SPOUT_SYPHON_APP_SOURCES src/platform/windows/spout_bridge.cpp)
    set(NOZZLE_BRIDGE_EXECUTABLE_NAME nozzle-spout)
else()
    message(FATAL_ERROR "unsupported platform")
endif()

add_library(nozzle_spout_syphon_app STATIC ${NOZZLE_SPOUT_SYPHON_APP_SOURCES})
target_include_directories(nozzle_spout_syphon_app PUBLIC src)
target_link_libraries(nozzle_spout_syphon_app PUBLIC nozzle)
if(APPLE)
    get_filename_component(SYPHON_FRAMEWORK_PARENT "${SYPHON_FRAMEWORK_DIR}" DIRECTORY)
    target_compile_options(nozzle_spout_syphon_app PRIVATE "-F${SYPHON_FRAMEWORK_PARENT}")
    target_link_libraries(nozzle_spout_syphon_app PUBLIC
        "-F${SYPHON_FRAMEWORK_PARENT}"
        "-framework Syphon"
        "-framework Foundation"
        "-framework Metal"
    )
endif()
if(WIN32)
    target_link_libraries(nozzle_spout_syphon_app PUBLIC SpoutDX_static)
    target_include_directories(nozzle_spout_syphon_app PRIVATE
        "${SPOUT2_SOURCE_DIR}/SPOUTSDK/SpoutDirectX/SpoutDX"
        "${SPOUT2_SOURCE_DIR}/SPOUTSDK/SpoutGL"
    )
endif()
target_compile_options(nozzle_spout_syphon_app PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
)

target_compile_definitions(nozzle_spout_syphon_app PUBLIC
    NOZZLE_SPOUT_SYPHON_VERSION="${PROJECT_VERSION}"
)

add_executable(${NOZZLE_BRIDGE_EXECUTABLE_NAME} src/main.cpp)
target_link_libraries(${NOZZLE_BRIDGE_EXECUTABLE_NAME} PRIVATE nozzle_spout_syphon_app)
target_compile_options(${NOZZLE_BRIDGE_EXECUTABLE_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
)

if(APPLE)
    target_link_options(${NOZZLE_BRIDGE_EXECUTABLE_NAME} PRIVATE
        "-F${SYPHON_FRAMEWORK_PARENT}"
        "LINKER:-rpath,@executable_path/../Frameworks"
    )
    set_target_properties(${NOZZLE_BRIDGE_EXECUTABLE_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER org.nozzle-io.syphon
        MACOSX_BUNDLE_BUNDLE_NAME "Nozzle Syphon"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
    add_custom_command(TARGET ${NOZZLE_BRIDGE_EXECUTABLE_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_DIR:${NOZZLE_BRIDGE_EXECUTABLE_NAME}>/Contents/Frameworks"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_BUNDLE_DIR:${NOZZLE_BRIDGE_EXECUTABLE_NAME}>/Contents/Frameworks/Syphon.framework"
        COMMAND /usr/bin/ditto
            "${SYPHON_FRAMEWORK_DIR}"
            "$<TARGET_BUNDLE_DIR:${NOZZLE_BRIDGE_EXECUTABLE_NAME}>/Contents/Frameworks/Syphon.framework"
    )
endif()

if(NOZZLE_SPOUT_SYPHON_BUILD_TESTS)
    include(CTest)
    add_executable(nozzle_spout_syphon_tests tests/test_main.cpp)
    target_link_libraries(nozzle_spout_syphon_tests PRIVATE nozzle_spout_syphon_app)
    if(APPLE)
        target_link_options(nozzle_spout_syphon_tests PRIVATE
            "-F${SYPHON_FRAMEWORK_PARENT}"
            "LINKER:-rpath,${SYPHON_FRAMEWORK_PARENT}"
        )
    endif()
    target_compile_options(nozzle_spout_syphon_tests PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wall -Wextra -Wpedantic -fno-exceptions>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -fno-exceptions>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHs-c->
    )
    add_test(NAME nozzle_spout_syphon_tests COMMAND nozzle_spout_syphon_tests)
endif()
