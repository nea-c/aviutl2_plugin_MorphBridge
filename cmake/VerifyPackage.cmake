if(NOT DEFINED STAGE)
  message(FATAL_ERROR "STAGE is required")
endif()
if(NOT DEFINED RELEASE)
  message(FATAL_ERROR "RELEASE is required")
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

file(READ "${STAGE}/package.ini" package_ini)
foreach(expected_line IN ITEMS
    "version=${RELEASE}"
    "information=MorphBridge ${RELEASE} (AviUtl2 2.1.8+, Windows x64)"
    "file=MorphBridge-${RELEASE}.au2pkg.zip")
  string(FIND "${package_ini}" "${expected_line}" position)
  if(position EQUAL -1)
    message(FATAL_ERROR "package.ini is missing: ${expected_line}")
  endif()
endforeach()

file(READ "${STAGE}/package.txt" package_text)
string(FIND "${package_text}" "MorphBridge ${RELEASE}" position)
if(position EQUAL -1)
  message(FATAL_ERROR "package.txt is missing release ${RELEASE}")
endif()
