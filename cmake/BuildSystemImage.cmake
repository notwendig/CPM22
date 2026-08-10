foreach(_required BOOT_FILE CPM_FILE BIOS_FILE OUTPUT_FILE EXPECTED_BOOT_SIZE EXPECTED_CPM_SIZE MAX_SYSTEM_SIZE)
    if(NOT DEFINED ${_required})
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

foreach(_input BOOT_FILE CPM_FILE BIOS_FILE)
    if(NOT EXISTS "${${_input}}")
        message(FATAL_ERROR "Input file does not exist: ${${_input}}")
    endif()
endforeach()

file(SIZE "${BOOT_FILE}" _boot_size)
file(SIZE "${CPM_FILE}" _cpm_size)
file(SIZE "${BIOS_FILE}" _bios_size)

if(NOT _boot_size EQUAL EXPECTED_BOOT_SIZE)
    message(FATAL_ERROR
        "Invalid boot sector size: ${_boot_size} bytes; expected ${EXPECTED_BOOT_SIZE} bytes"
    )
endif()

if(NOT _cpm_size EQUAL EXPECTED_CPM_SIZE)
    message(FATAL_ERROR
        "Invalid CP/M CCP+BDOS size: ${_cpm_size} bytes; expected ${EXPECTED_CPM_SIZE} bytes"
    )
endif()

math(EXPR _system_size "${_boot_size} + ${_cpm_size} + ${_bios_size}")
if(_system_size GREATER MAX_SYSTEM_SIZE)
    message(FATAL_ERROR
        "CP/M system image is too large: ${_system_size} bytes; maximum is ${MAX_SYSTEM_SIZE} bytes"
    )
endif()

get_filename_component(_output_dir "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_output_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E cat "${BOOT_FILE}" "${CPM_FILE}" "${BIOS_FILE}"
    OUTPUT_FILE "${OUTPUT_FILE}"
    COMMAND_ERROR_IS_FATAL ANY
)

file(SIZE "${OUTPUT_FILE}" _written_size)
if(NOT _written_size EQUAL _system_size)
    message(FATAL_ERROR
        "Generated system image has ${_written_size} bytes; expected ${_system_size} bytes"
    )
endif()

message(STATUS
    "CP/M system image: boot=${_boot_size}, cpm=${_cpm_size}, bios=${_bios_size}, total=${_system_size} bytes"
)
