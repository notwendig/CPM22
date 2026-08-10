if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_FILE OR NOT DEFINED ARRAY_NAME)
    message(FATAL_ERROR "INPUT_FILE, OUTPUT_FILE and ARRAY_NAME are required")
endif()

file(READ "${INPUT_FILE}" _hex HEX)
string(LENGTH "${_hex}" _hex_length)
math(EXPR _byte_count "${_hex_length} / 2")

file(WRITE "${OUTPUT_FILE}"
    "#pragma once\n#include <cstddef>\n#include <cstdint>\n\n"
    "inline constexpr std::uint8_t ${ARRAY_NAME}[] = {\n")

set(_offset 0)
while(_offset LESS _hex_length)
    set(_line "    ")
    foreach(_column RANGE 0 15)
        if(_offset GREATER_EQUAL _hex_length)
            break()
        endif()
        string(SUBSTRING "${_hex}" ${_offset} 2 _byte)
        string(APPEND _line "0x${_byte}, ")
        math(EXPR _offset "${_offset} + 2")
    endforeach()
    file(APPEND "${OUTPUT_FILE}" "${_line}\n")
endwhile()

file(APPEND "${OUTPUT_FILE}"
    "};\ninline constexpr std::size_t ${ARRAY_NAME}_length = sizeof(${ARRAY_NAME});\n")
