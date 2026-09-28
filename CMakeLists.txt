cmake_minimum_required(VERSION 3.21)

project(
    CPM22
    VERSION 1.0.0
    DESCRIPTION "CP/M 2.2 emulator and host-filesystem-backed Z80 environment"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CTest)

option(CPM22_WARNINGS_AS_ERRORS "Treat CPM22 compiler warnings as errors" OFF)

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Z80 emulator library - Git submodule
#
# Z80 is maintained as a git submodule of CPM22:
#
#   CPM22/Z80
#
# Initialise it after cloning with:
#
#   git submodule update --init --recursive
# ---------------------------------------------------------------------------

set(
    Z80_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/Z80"
    CACHE PATH
    "Path to the Z80 git submodule"
)

if(NOT EXISTS "${Z80_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Z80 git submodule is missing.\n"
        "Run:\n"
        "  git submodule update --init --recursive\n"
        "Expected: ${Z80_SOURCE_DIR}"
    )
endif()

set(Z80_BUILD_SHARED OFF CACHE BOOL "Build shared Z80 library" FORCE)
set(Z80_BUILD_STATIC ON CACHE BOOL "Build static Z80 library" FORCE)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
    # Treat Z80's public headers as external/system headers when consumed
    # by CPM22. This keeps CPM22's warning policy strict without reporting
    # warnings that belong to the separate Z80 project.
    add_subdirectory(
        "${Z80_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/Z80"
        SYSTEM
    )
else()
    add_subdirectory(
        "${Z80_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/Z80"
    )
endif()

if(NOT TARGET Z80::Z80)
    message(FATAL_ERROR
        "Z80 submodule did not provide the required target Z80::Z80"
    )
endif()

# The CP/M system image and all assembly-only utility programs are built in
# their own subdirectory. The subdirectory exports only the paths needed by
# the host emulator target below.
add_subdirectory(cpm22)

# ---------------------------------------------------------------------------
# Host-side CP/M emulator
# ---------------------------------------------------------------------------

set(CPM22_SOURCES
    src/Z80CPM.cpp
    src/server.cpp
    src/biosapi.cpp
    src/biosapi.h
    src/cpm.h
    src/monitor.cpp
    src/monitor.h
    src/zilogz80.cpp
    src/zilogz80.h
    "${CPM22_BOOT_HEADER}"
)

# Keep the CP/M assembly sources visible in Qt Creator without trying to compile
# them as host assembler files.
set_source_files_properties(${CPM22_ASM_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(CPM22
    ${CPM22_SOURCES}
    ${CPM22_ASM_SOURCES}
)

add_dependencies(CPM22 CPM22_images)

set_target_properties(CPM22 PROPERTIES
    OUTPUT_NAME "cpm-2.2"
)

target_compile_features(CPM22 PRIVATE cxx_std_20)
set_target_properties(CPM22 PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(CPM22 PRIVATE
    CPMSYS="${CPM22_SYSTEM_FILENAME}"
    BOOTDISK="./disks/A"
    BOOTSECTOR="${CPM22_SYSTEM_IMAGE}"
    CPM22_DEFAULT_DISKPATH="${CMAKE_CURRENT_BINARY_DIR}"
    $<$<CONFIG:Debug>:DEBUG>
)

target_include_directories(CPM22 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CPM22_GENERATED_DIR}"
)

target_link_libraries(CPM22 PRIVATE
    Z80::Z80
    Threads::Threads
)

target_compile_options(CPM22 PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>>:-O0;-g3>
)

if(CPM22_WARNINGS_AS_ERRORS)
    target_compile_options(CPM22 PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Werror>
    )
endif()

# Convenient target for terminal use:
#   cmake --build --preset debug --target run
add_custom_target(run
    COMMAND "$<TARGET_FILE:CPM22>" --diskpath "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS CPM22
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    USES_TERMINAL
)
