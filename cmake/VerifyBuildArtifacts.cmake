cmake_minimum_required(VERSION 3.21)

foreach(_var IN ITEMS BOOT_FILE CPM_FILE SYSTEM_IMAGE ZEXDOC_FILE ZEXALL_FILE EXECUTABLE)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "${_var} is required")
    endif()
    if(NOT EXISTS "${${_var}}")
        message(FATAL_ERROR "Required build artifact does not exist: ${${_var}}")
    endif()
endforeach()

file(SIZE "${BOOT_FILE}" _boot_size)
file(SIZE "${CPM_FILE}" _cpm_size)
file(SIZE "${SYSTEM_IMAGE}" _system_size)
file(SIZE "${ZEXDOC_FILE}" _zexdoc_size)
file(SIZE "${ZEXALL_FILE}" _zexall_size)

if(NOT _boot_size EQUAL EXPECTED_BOOT_SIZE)
    message(FATAL_ERROR "boot.cim size is ${_boot_size}, expected ${EXPECTED_BOOT_SIZE}")
endif()

if(NOT _cpm_size EQUAL EXPECTED_CPM_SIZE)
    message(FATAL_ERROR "cpm22.cim size is ${_cpm_size}, expected ${EXPECTED_CPM_SIZE}")
endif()

if(_system_size GREATER MAX_SYSTEM_SIZE)
    message(FATAL_ERROR "CP/M system image is ${_system_size} bytes, maximum is ${MAX_SYSTEM_SIZE}")
endif()

if(_zexdoc_size EQUAL 0 OR _zexall_size EQUAL 0)
    message(FATAL_ERROR "ZEX test program artifact is empty")
endif()

message(STATUS
    "Verified CPM22 artifacts: boot=${_boot_size}, cpm=${_cpm_size}, "
    "system=${_system_size}, zexdoc=${_zexdoc_size}, zexall=${_zexall_size} bytes"
)
