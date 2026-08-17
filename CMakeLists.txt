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

# ---------------------------------------------------------------------------
# zmac assembler
# ---------------------------------------------------------------------------

find_program(
    ZMAC_EXECUTABLE
    NAMES zmac
    HINTS "$ENV{HOME}/bin"
    REQUIRED
)

set(CPM22_CPM_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/cpm")
set(CPM22_ZOUT_DIR      "${CMAKE_CURRENT_BINARY_DIR}/zout")
set(CPM22_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(CPM22_DISKS_DIR     "${CMAKE_CURRENT_BINARY_DIR}/disks")
set(CPM22_DRIVE_A_DIR   "${CPM22_DISKS_DIR}/A")
set(CPM22_SYSTEM_IMAGE  "${CPM22_DRIVE_A_DIR}/.cpm22.sys")
set(CPM22_BOOT_HEADER   "${CPM22_GENERATED_DIR}/boot.h")

# Seed the out-of-source runtime tree with the existing CP/M disk contents.
# Generated .cpm22.sys / ZEX .com files are then added to this copied tree.
file(GLOB_RECURSE CPM22_DISK_SEED_FILES CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    "${CMAKE_CURRENT_SOURCE_DIR}/disks/*"
)

set(CPM22_RUNTIME_SEED_STAMP "${CMAKE_CURRENT_BINARY_DIR}/.runtime-seeded")
add_custom_command(
    OUTPUT "${CPM22_RUNTIME_SEED_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CPM22_DISKS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/disks"
            "${CPM22_DISKS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${CPM22_RUNTIME_SEED_STAMP}"
    DEPENDS ${CPM22_DISK_SEED_FILES}
    COMMENT "Copying CP/M runtime disks to the build tree"
    VERBATIM
)

# Match the original Makefile constants.
set(CPM22_BOOT_SIZE 6656)
set(CPM22_SECTOR_SIZE 128)
set(CPM22_CCP_BDOS_SIZE 5632)

function(cpm22_assemble output_name source_file)
    set(options ZMAC_COMPAT)
    set(oneValueArgs)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(CPM22ASM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_cim "${CPM22_ZOUT_DIR}/${output_name}.cim")
    set(_lst "${CPM22_ZOUT_DIR}/${output_name}.lst")
    set(_zmac_args -czfm)

    if(CPM22ASM_ZMAC_COMPAT)
        list(APPEND _zmac_args --zmac)
    endif()

    add_custom_command(
        OUTPUT
            "${_cim}"
            "${_lst}"
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${CPM22_ZOUT_DIR}"
        COMMAND
            "${ZMAC_EXECUTABLE}"
            ${_zmac_args}
            -I "${CPM22_CPM_DIR}"
            --od "${CPM22_ZOUT_DIR}"
            --oo cim,lst
            "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}"
            ${CPM22ASM_DEPENDS}
        COMMENT "Assembling ${source_file}"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )

    set(${output_name}_CIM "${_cim}" PARENT_SCOPE)
    set(${output_name}_LST "${_lst}" PARENT_SCOPE)
endfunction()

# CP/M operating system and BIOS.
cpm22_assemble(
    cpm22
    cpm/cpm22.asm
    DEPENDS "${CPM22_CPM_DIR}/cpm.inc"
)

cpm22_assemble(
    bios
    cpm/bios.asm
    DEPENDS "${CPM22_CPM_DIR}/cpm.inc"
)

# Preserve the old Makefile ordering: boot is assembled after CCP/BDOS + BIOS.
cpm22_assemble(
    boot
    cpm/boot.asm
    DEPENDS
        "${CPM22_CPM_DIR}/cpm.inc"
        "${cpm22_CIM}"
        "${bios_CIM}"
)

# CP/M test programs. The old Makefile assembled these in zmac compatibility
# mode and copied the resulting raw .cim images to drive A as .com files.
cpm22_assemble(zexdoc cpm/zexdoc.asm ZMAC_COMPAT)
cpm22_assemble(zexall cpm/zexall.asm ZMAC_COMPAT)

# ---------------------------------------------------------------------------
# Generated boot header (replaces the old srec_cat generated src/boot.cpp)
# ---------------------------------------------------------------------------

add_custom_command(
    OUTPUT "${CPM22_BOOT_HEADER}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${CPM22_GENERATED_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DINPUT_FILE=${boot_CIM}"
        "-DOUTPUT_FILE=${CPM22_BOOT_HEADER}"
        -DARRAY_NAME=boot
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BinaryToHeader.cmake"
    DEPENDS
        "${boot_CIM}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BinaryToHeader.cmake"
    COMMENT "Generating embedded boot loader header"
    VERBATIM
)

# ---------------------------------------------------------------------------
# CP/M system tracks and drive-A utility programs
# ---------------------------------------------------------------------------

add_custom_command(
    OUTPUT "${CPM22_SYSTEM_IMAGE}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${CPM22_DRIVE_A_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DBOOT_FILE=${boot_CIM}"
        "-DCPM_FILE=${cpm22_CIM}"
        "-DBIOS_FILE=${bios_CIM}"
        "-DOUTPUT_FILE=${CPM22_SYSTEM_IMAGE}"
        "-DEXPECTED_BOOT_SIZE=${CPM22_SECTOR_SIZE}"
        "-DEXPECTED_CPM_SIZE=${CPM22_CCP_BDOS_SIZE}"
        "-DMAX_SYSTEM_SIZE=${CPM22_BOOT_SIZE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildSystemImage.cmake"
    DEPENDS
        "${boot_CIM}"
        "${cpm22_CIM}"
        "${bios_CIM}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildSystemImage.cmake"
        "${CPM22_RUNTIME_SEED_STAMP}"
    COMMENT "Creating CP/M 2.2 system tracks (.cpm22.sys)"
    VERBATIM
)

set(CPM22_ZEXDOC_COM "${CPM22_DRIVE_A_DIR}/zexdoc.com")
set(CPM22_ZEXALL_COM "${CPM22_DRIVE_A_DIR}/zexall.com")

add_custom_command(
    OUTPUT "${CPM22_ZEXDOC_COM}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CPM22_DRIVE_A_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${zexdoc_CIM}" "${CPM22_ZEXDOC_COM}"
    DEPENDS "${zexdoc_CIM}" "${CPM22_RUNTIME_SEED_STAMP}"
    COMMENT "Installing zexdoc.com on drive A"
    VERBATIM
)

add_custom_command(
    OUTPUT "${CPM22_ZEXALL_COM}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CPM22_DRIVE_A_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${zexall_CIM}" "${CPM22_ZEXALL_COM}"
    DEPENDS "${zexall_CIM}" "${CPM22_RUNTIME_SEED_STAMP}"
    COMMENT "Installing zexall.com on drive A"
    VERBATIM
)

add_custom_target(
    CPM22_images ALL
    DEPENDS
        "${CPM22_SYSTEM_IMAGE}"
        "${CPM22_ZEXDOC_COM}"
        "${CPM22_ZEXALL_COM}"
)

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
set(CPM22_ASM_SOURCES
    cpm/cpm22.asm
    cpm/bios.asm
    cpm/boot.asm
    cpm/cpm.inc
    cpm/zexdoc.asm
    cpm/zexall.asm
)
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
    CPMSYS=".cpm22.sys"
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

if(BUILD_TESTING)
    add_test(
        NAME CPM22.verify-build-artifacts
        COMMAND
            "${CMAKE_COMMAND}"
            "-DBOOT_FILE=${boot_CIM}"
            "-DCPM_FILE=${cpm22_CIM}"
            "-DSYSTEM_IMAGE=${CPM22_SYSTEM_IMAGE}"
            "-DZEXDOC_FILE=${CPM22_ZEXDOC_COM}"
            "-DZEXALL_FILE=${CPM22_ZEXALL_COM}"
            "-DEXECUTABLE=$<TARGET_FILE:CPM22>"
            "-DEXPECTED_BOOT_SIZE=${CPM22_SECTOR_SIZE}"
            "-DEXPECTED_CPM_SIZE=${CPM22_CCP_BDOS_SIZE}"
            "-DMAX_SYSTEM_SIZE=${CPM22_BOOT_SIZE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyBuildArtifacts.cmake"
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

