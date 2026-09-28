# ---------------------------------------------------------------------------
# CP/M 2.2 system tracks
# ---------------------------------------------------------------------------

find_program(
    ZMAC_EXECUTABLE
    NAMES zmac
    HINTS "$ENV{HOME}/bin"
    REQUIRED
)

set(CPM22_CPM_DIR         "${CMAKE_CURRENT_SOURCE_DIR}")
set(CPM22_ZOUT_DIR        "${PROJECT_BINARY_DIR}/zout")
set(CPM22_GENERATED_DIR   "${PROJECT_BINARY_DIR}/generated")
set(CPM22_DISKS_DIR       "${PROJECT_BINARY_DIR}/disks")
set(CPM22_DRIVE_A_DIR     "${CPM22_DISKS_DIR}/A")
set(CPM22_SYSTEM_FILENAME ".cpm22.sys")
set(CPM22_SYSTEM_IMAGE    "${CPM22_DRIVE_A_DIR}/${CPM22_SYSTEM_FILENAME}")
set(CPM22_BOOT_HEADER     "${CPM22_GENERATED_DIR}/boot.h")

# CP/M 2.2 system layout. Hexadecimal constants are kept in the same form as
# the assembly layout; all derived sizes are calculated by CMake.
set(CPM22_SYSTEM_TRACKS     2)
set(CPM22_SECTORS_PER_TRACK 26)
set(CPM22_SECTOR_SIZE       128)
set(CPM22_BOOT_SIZE         128)
set(CPM22_CCP_SIZE          0x806)
set(CPM22_BDOS_SIZE         0xDFA)
set(CPM22_BIOS_SIZE         0x280)

math(EXPR CPM22_CCP_BDOS_SIZE
    "${CPM22_CCP_SIZE} + ${CPM22_BDOS_SIZE}"
)
math(EXPR CPM22_MAX_SYSTEM_SIZE
    "${CPM22_SYSTEM_TRACKS} * ${CPM22_SECTORS_PER_TRACK} * ${CPM22_SECTOR_SIZE}"
)
math(EXPR CPM22_FIXED_SYSTEM_SIZE
    "${CPM22_BOOT_SIZE} + ${CPM22_CCP_SIZE} + ${CPM22_BDOS_SIZE}"
)
math(EXPR CPM22_MAX_BIOS_SIZE
    "${CPM22_MAX_SYSTEM_SIZE} - ${CPM22_FIXED_SYSTEM_SIZE}"
)

if(NOT CPM22_BOOT_SIZE EQUAL CPM22_SECTOR_SIZE)
    message(FATAL_ERROR
        "boot.asm must occupy exactly one ${CPM22_SECTOR_SIZE}-byte sector"
    )
endif()

if(CPM22_FIXED_SYSTEM_SIZE GREATER CPM22_MAX_SYSTEM_SIZE)
    message(FATAL_ERROR
        "boot.asm + ccp.asm + bdos.asm already exceed the "
        "${CPM22_MAX_SYSTEM_SIZE}-byte system area"
    )
endif()

message(STATUS
    "CP/M system area: boot=${CPM22_BOOT_SIZE}, "
    "CCP=${CPM22_CCP_SIZE}, BDOS=${CPM22_BDOS_SIZE}, "
    "BIOS<=${CPM22_MAX_BIOS_SIZE}, total<=${CPM22_MAX_SYSTEM_SIZE} bytes"
)

# Seed the out-of-source runtime tree with the existing CP/M disk contents.
# Generated system tracks and ZEX .com files are added to this copied tree.
file(GLOB_RECURSE CPM22_DISK_SEED_FILES CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    "${PROJECT_SOURCE_DIR}/disks/*"
)

set(CPM22_RUNTIME_SEED_STAMP "${PROJECT_BINARY_DIR}/.runtime-seeded")
add_custom_command(
    OUTPUT "${CPM22_RUNTIME_SEED_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CPM22_DISKS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${PROJECT_SOURCE_DIR}/disks"
            "${CPM22_DISKS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${CPM22_RUNTIME_SEED_STAMP}"
    DEPENDS ${CPM22_DISK_SEED_FILES}
    COMMENT "Copying CP/M runtime disks to the build tree"
    VERBATIM
)

function(cpm22_assemble output_name source_file)
    set(options ZMAC_COMPAT)
    set(oneValueArgs)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(CPM22ASM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(CPM22ASM_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown arguments for ${source_file}: ${CPM22ASM_UNPARSED_ARGUMENTS}"
        )
    endif()

    set(_source "${CPM22_CPM_DIR}/${source_file}")
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
            "${_source}"
        DEPENDS
            "${_source}"
            ${CPM22ASM_DEPENDS}
        COMMENT "Assembling cpm22/${source_file}"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )

    set(${output_name}_CIM "${_cim}" PARENT_SCOPE)
    set(${output_name}_LST "${_lst}" PARENT_SCOPE)
endfunction()

# Assemble all four parts of the system image independently.
cpm22_assemble(
    ccp
    ccp.asm
    DEPENDS "${CPM22_CPM_DIR}/cpm.inc"
)

cpm22_assemble(
    bdos
    bdos.asm
    DEPENDS "${CPM22_CPM_DIR}/cpm.inc"
)

cpm22_assemble(
    bios
    bios.asm
    DEPENDS "${CPM22_CPM_DIR}/cpm.inc"
)

cpm22_assemble(
    boot
    boot.asm
    DEPENDS
        "${CPM22_CPM_DIR}/cpm.inc"
        "${ccp_CIM}"
        "${bdos_CIM}"
        "${bios_CIM}"
)

# CP/M test programs. The old Makefile assembled these in zmac compatibility
# mode and copied the resulting raw .cim images to drive A as .com files.
cpm22_assemble(zexdoc zexdoc.asm ZMAC_COMPAT)
cpm22_assemble(zexall zexall.asm ZMAC_COMPAT)

add_custom_command(
    OUTPUT "${CPM22_BOOT_HEADER}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${CPM22_GENERATED_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DINPUT_FILE=${boot_CIM}"
        "-DOUTPUT_FILE=${CPM22_BOOT_HEADER}"
        -DARRAY_NAME=boot
        -P "${PROJECT_SOURCE_DIR}/cmake/BinaryToHeader.cmake"
    DEPENDS
        "${boot_CIM}"
        "${PROJECT_SOURCE_DIR}/cmake/BinaryToHeader.cmake"
    COMMENT "Generating embedded boot loader header"
    VERBATIM
)

add_custom_command(
    OUTPUT "${CPM22_SYSTEM_IMAGE}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${CPM22_DRIVE_A_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DBOOT_FILE=${boot_CIM}"
        "-DCCP_FILE=${ccp_CIM}"
        "-DBDOS_FILE=${bdos_CIM}"
        "-DBIOS_FILE=${bios_CIM}"
        "-DOUTPUT_FILE=${CPM22_SYSTEM_IMAGE}"
        "-DEXPECTED_BOOT_SIZE=${CPM22_BOOT_SIZE}"
        "-DEXPECTED_CCP_SIZE=${CPM22_CCP_SIZE}"
        "-DEXPECTED_BDOS_SIZE=${CPM22_BDOS_SIZE}"
        "-DMAX_SYSTEM_SIZE=${CPM22_MAX_SYSTEM_SIZE}"
        -P "${PROJECT_SOURCE_DIR}/cmake/BuildSystemImage.cmake"
    DEPENDS
        "${boot_CIM}"
        "${ccp_CIM}"
        "${bdos_CIM}"
        "${bios_CIM}"
        "${PROJECT_SOURCE_DIR}/cmake/BuildSystemImage.cmake"
        "${CPM22_RUNTIME_SEED_STAMP}"
    COMMENT "Creating CP/M 2.2 system tracks (${CPM22_SYSTEM_FILENAME})"
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
        "${CPM22_BOOT_HEADER}"
        "${CPM22_SYSTEM_IMAGE}"
        "${CPM22_ZEXDOC_COM}"
        "${CPM22_ZEXALL_COM}"
)

if(BUILD_TESTING)
    add_test(
        NAME CPM22.verify-build-artifacts
        COMMAND
            "${CMAKE_COMMAND}"
            "-DBOOT_FILE=${boot_CIM}"
            "-DCCP_FILE=${ccp_CIM}"
            "-DBDOS_FILE=${bdos_CIM}"
            "-DBIOS_FILE=${bios_CIM}"
            "-DSYSTEM_IMAGE=${CPM22_SYSTEM_IMAGE}"
            "-DZEXDOC_FILE=${CPM22_ZEXDOC_COM}"
            "-DZEXALL_FILE=${CPM22_ZEXALL_COM}"
            "-DEXECUTABLE=$<TARGET_FILE:CPM22>"
            "-DEXPECTED_BOOT_SIZE=${CPM22_BOOT_SIZE}"
            "-DEXPECTED_CCP_SIZE=${CPM22_CCP_SIZE}"
            "-DEXPECTED_BDOS_SIZE=${CPM22_BDOS_SIZE}"
            "-DMAX_SYSTEM_SIZE=${CPM22_MAX_SYSTEM_SIZE}"
            -P "${PROJECT_SOURCE_DIR}/cmake/VerifyBuildArtifacts.cmake"
    )
endif()

set(CPM22_ASM_SOURCES
    "${CPM22_CPM_DIR}/ccp.asm"
    "${CPM22_CPM_DIR}/bdos.asm"
    "${CPM22_CPM_DIR}/bios.asm"
    "${CPM22_CPM_DIR}/boot.asm"
    "${CPM22_CPM_DIR}/cpm.inc"
    "${CPM22_CPM_DIR}/zexdoc.asm"
    "${CPM22_CPM_DIR}/zexall.asm"
)

# Export only the artifacts consumed by the host-side target.
set(CPM22_ASM_SOURCES       "${CPM22_ASM_SOURCES}"       PARENT_SCOPE)
set(CPM22_BOOT_HEADER       "${CPM22_BOOT_HEADER}"       PARENT_SCOPE)
set(CPM22_GENERATED_DIR     "${CPM22_GENERATED_DIR}"     PARENT_SCOPE)
set(CPM22_SYSTEM_FILENAME   "${CPM22_SYSTEM_FILENAME}"   PARENT_SCOPE)
set(CPM22_SYSTEM_IMAGE      "${CPM22_SYSTEM_IMAGE}"      PARENT_SCOPE)
