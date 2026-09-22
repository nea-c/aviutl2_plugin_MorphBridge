if(NOT DEFINED STAGE)
  message(FATAL_ERROR "STAGE is required")
endif()

set(expected
  "Plugin/MorphBridge/MorphBridge.aux2"
  "package.ini"
  "package.txt")
foreach(path IN LISTS expected)
  if(NOT EXISTS "${STAGE}/${path}")
    message(FATAL_ERROR "Missing package file: ${path}")
  endif()
endforeach()

file(GLOB_RECURSE staged RELATIVE "${STAGE}" "${STAGE}/*")
list(FILTER staged EXCLUDE REGEX "(^|/)\\.gitkeep$")
list(SORT staged)
list(SORT expected)
if(NOT staged STREQUAL expected)
  message(FATAL_ERROR "Unexpected package layout: ${staged}")
endif()

foreach(path IN LISTS staged)
  if(path MATCHES "\\.(obj2|mod2|auf2|pdb|cpp|hpp|hlsl)$")
    message(FATAL_ERROR "Forbidden package file: ${path}")
  endif()
endforeach()
