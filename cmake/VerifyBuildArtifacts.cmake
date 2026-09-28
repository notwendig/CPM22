cmake_minimum_required(VERSION 3.21)

foreach(_var IN ITEMS
    BOOT_FILE
    CCP_FILE
    BDOS_FILE
    BIOS_FILE
    SYSTEM_IMAGE
    ZEXDOC_FILE
    ZEXALL_FILE
    EXECUTABLE
)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "${_var} is required")
    endif()
    if(NOT EXISTS "${${_var}}")
        message(FATAL_ERROR "Required build artifact does not exist: ${${_var}}")
    endif()
endforeach()

file(SIZE "${BOOT_FILE}" _boot_size)
file(SIZE "${CCP_FILE}" _ccp_size)
file(SIZE "${BDOS_FILE}" _bdos_size)
file(SIZE "${BIOS_FILE}" _bios_size)
file(SIZE "${SYSTEM_IMAGE}" _system_size)
file(SIZE "${ZEXDOC_FILE}" _zexdoc_size)
file(SIZE "${ZEXALL_FILE}" _zexall_size)

if(NOT _boot_size EQUAL EXPECTED_BOOT_SIZE)
    message(FATAL_ERROR "boot.cim size is ${_boot_size}, expected ${EXPECTED_BOOT_SIZE}")
endif()

if(NOT _ccp_size EQUAL EXPECTED_CCP_SIZE)
    message(FATAL_ERROR "ccp.cim size is ${_ccp_size}, expected ${EXPECTED_CCP_SIZE}")
endif()

if(NOT _bdos_size EQUAL EXPECTED_BDOS_SIZE)
    message(FATAL_ERROR "bdos.cim size is ${_bdos_size}, expected ${EXPECTED_BDOS_SIZE}")
endif()

math(EXPR _expected_system_size
    "${_boot_size} + ${_ccp_size} + ${_bdos_size} + ${_bios_size}"
)

if(NOT _system_size EQUAL _expected_system_size)
    message(FATAL_ERROR
        "CP/M system image is ${_system_size} bytes; "
        "boot+CCP+BDOS+BIOS require ${_expected_system_size} bytes"
    )
endif()

if(_system_size GREATER MAX_SYSTEM_SIZE)
    message(FATAL_ERROR "CP/M system image is ${_system_size} bytes, maximum is ${MAX_SYSTEM_SIZE}")
endif()

if(_zexdoc_size EQUAL 0 OR _zexall_size EQUAL 0)
    message(FATAL_ERROR "ZEX test program artifact is empty")
endif()

message(STATUS
    "Verified CPM22 artifacts: boot=${_boot_size}, ccp=${_ccp_size}, "
    "bdos=${_bdos_size}, bios=${_bios_size}, system=${_system_size}, "
    "zexdoc=${_zexdoc_size}, zexall=${_zexall_size} bytes"
)
