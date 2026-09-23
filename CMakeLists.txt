cmake_minimum_required(VERSION 3.28)

project(MorphBridge VERSION 0.1.19 LANGUAGES CXX)

include(cmake/PlatformGuard.cmake)
morph_bridge_require_windows_x64("${WIN32}" "${CMAKE_SIZEOF_VOID_P}")

include(CTest)

add_library(aviutl2_sdk INTERFACE)
target_include_directories(
  aviutl2_sdk INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/aviutl2_sdk")
target_compile_definitions(aviutl2_sdk INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(aviutl2_sdk INTERFACE /wd4828)

add_library(
  morph_bridge_core
  STATIC
  src/domain/types.cpp
  src/domain/neighbor_resolver.cpp
  src/cache/signature.cpp
  src/cache/cache_state.cpp
  src/capture/endpoint_capture.cpp
  src/render/canvas.cpp
  src/render/sdf_encoding.cpp
  src/render/standard_transform.cpp
  src/render/sdf_plan.cpp
  src/render/gpu_renderer.cpp
  src/plugin/filter_object.cpp)
target_include_directories(morph_bridge_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(morph_bridge_core PUBLIC cxx_std_20)
target_compile_options(morph_bridge_core PRIVATE /utf-8 /W4 /permissive-)
set_target_properties(morph_bridge_core PROPERTIES CXX_SCAN_FOR_MODULES OFF)

add_library(MorphBridge MODULE src/plugin/plugin_entry.cpp)
target_compile_features(MorphBridge PRIVATE cxx_std_20)
target_compile_options(MorphBridge PRIVATE /utf-8)
target_link_libraries(MorphBridge PRIVATE aviutl2_sdk morph_bridge_core)
set_target_properties(MorphBridge PROPERTIES PREFIX "" SUFFIX ".aux2")
set_target_properties(MorphBridge PROPERTIES CXX_SCAN_FOR_MODULES OFF)

find_program(FXC_EXECUTABLE NAMES fxc.exe fxc REQUIRED)
set(MORPH_BRIDGE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(MORPH_BRIDGE_SHADER_OUTPUTS)
foreach(shader IN ITEMS seed jump_flood finalize morph)
  if(shader STREQUAL "morph")
    set(shader_profile ps_5_0)
  else()
    set(shader_profile cs_5_0)
  endif()
  set(shader_output "${MORPH_BRIDGE_GENERATED_DIR}/${shader}.cso")
  add_custom_command(
    OUTPUT "${shader_output}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${MORPH_BRIDGE_GENERATED_DIR}"
    COMMAND "${FXC_EXECUTABLE}" /nologo /WX /T ${shader_profile} /E main
            /I "${CMAKE_CURRENT_SOURCE_DIR}/src" /Fo "${shader_output}"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}.hlsl"
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}.hlsl"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/render/sdf_constants_shared.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/render/sdf_coverage_shared.h"
    VERBATIM)
  list(APPEND MORPH_BRIDGE_SHADER_OUTPUTS "${shader_output}")
endforeach()

set(MORPH_BRIDGE_SHADER_HEADER "${MORPH_BRIDGE_GENERATED_DIR}/shaders.hpp")
add_custom_command(
  OUTPUT "${MORPH_BRIDGE_SHADER_HEADER}"
  COMMAND "${CMAKE_COMMAND}"
          "-DINPUTS=${MORPH_BRIDGE_GENERATED_DIR}/seed.cso|${MORPH_BRIDGE_GENERATED_DIR}/jump_flood.cso|${MORPH_BRIDGE_GENERATED_DIR}/finalize.cso|${MORPH_BRIDGE_GENERATED_DIR}/morph.cso"
          "-DSYMBOLS=seed_cso|jump_flood_cso|finalize_cso|morph_cso"
          "-DOUTPUT=${MORPH_BRIDGE_SHADER_HEADER}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinary.cmake"
  DEPENDS ${MORPH_BRIDGE_SHADER_OUTPUTS}
  VERBATIM)
add_custom_target(morph_bridge_shaders DEPENDS "${MORPH_BRIDGE_SHADER_HEADER}")
add_dependencies(MorphBridge morph_bridge_shaders)
add_dependencies(morph_bridge_core morph_bridge_shaders)
target_include_directories(MorphBridge PRIVATE "${MORPH_BRIDGE_GENERATED_DIR}")
target_include_directories(morph_bridge_core PRIVATE "${MORPH_BRIDGE_GENERATED_DIR}")
target_link_libraries(morph_bridge_core PUBLIC aviutl2_sdk)

set(MORPH_BRIDGE_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/package")
set(MORPH_BRIDGE_STAGED_DLL
    "${MORPH_BRIDGE_PACKAGE_DIR}/Plugin/MorphBridge/MorphBridge.aux2")
add_custom_command(
  OUTPUT "${MORPH_BRIDGE_STAGED_DLL}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory
          "${MORPH_BRIDGE_PACKAGE_DIR}/Plugin/MorphBridge"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "$<TARGET_FILE:MorphBridge>" "${MORPH_BRIDGE_STAGED_DLL}"
  DEPENDS MorphBridge
  VERBATIM)
add_custom_target(stage_package ALL DEPENDS "${MORPH_BRIDGE_STAGED_DLL}")

set(MORPH_BRIDGE_PACKAGE_ZIP
    "${CMAKE_CURRENT_SOURCE_DIR}/MorphBridge-v${PROJECT_VERSION}.au2pkg.zip")
add_custom_target(
  package
  COMMAND "${CMAKE_COMMAND}" -E rm -f "${MORPH_BRIDGE_PACKAGE_ZIP}"
  COMMAND "${CMAKE_COMMAND}" -E tar cf "${MORPH_BRIDGE_PACKAGE_ZIP}"
          --format=zip package.ini package.txt Plugin/MorphBridge/MorphBridge.aux2
  WORKING_DIRECTORY "${MORPH_BRIDGE_PACKAGE_DIR}"
  DEPENDS stage_package
  VERBATIM)

if(BUILD_TESTING)
  add_executable(
    morph_bridge_tests
    tests/test_main.cpp
    tests/neighbor_resolver_test.cpp
    tests/signature_test.cpp
    tests/cache_state_test.cpp
    tests/endpoint_capture_test.cpp
    tests/canvas_test.cpp
    tests/sdf_encoding_test.cpp
    tests/standard_transform_test.cpp
    tests/sdf_plan_test.cpp
    tests/gpu_renderer_contract_test.cpp
    tests/plugin_registration_test.cpp
    tests/filter_object_test.cpp)
  target_compile_features(morph_bridge_tests PRIVATE cxx_std_20)
  target_compile_options(morph_bridge_tests PRIVATE /utf-8 /W4 /permissive-)
  target_link_libraries(morph_bridge_tests PRIVATE morph_bridge_core)
  set_target_properties(morph_bridge_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
  add_test(NAME morph_bridge_tests COMMAND morph_bridge_tests)
  add_test(
    NAME package_layout
    COMMAND "${CMAKE_COMMAND}" -DSTAGE=${MORPH_BRIDGE_PACKAGE_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyPackage.cmake")
endif()
