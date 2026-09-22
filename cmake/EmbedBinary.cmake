if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
  message(FATAL_ERROR "EmbedBinary.cmake requires INPUT, OUTPUT, and SYMBOL")
endif()

file(READ "${INPUT}" binary_hex HEX)
string(LENGTH "${binary_hex}" binary_hex_length)
math(EXPR binary_size "${binary_hex_length} / 2")
string(REGEX REPLACE "(..)" "0x\\1," binary_bytes "${binary_hex}")
file(WRITE "${OUTPUT}"
  "#pragma once\n#include <cstddef>\ninline constexpr unsigned char ${SYMBOL}[] = {${binary_bytes}};\ninline constexpr std::size_t ${SYMBOL}_size = ${binary_size};\n")
