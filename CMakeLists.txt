cmake_minimum_required(VERSION 3.28)

project(MorphBridge VERSION 0.1.0 LANGUAGES CXX)

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
  src/domain/neighbor_resolver.cpp)
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

if(BUILD_TESTING)
  add_executable(
    morph_bridge_tests
    tests/test_main.cpp
    tests/neighbor_resolver_test.cpp)
  target_compile_features(morph_bridge_tests PRIVATE cxx_std_20)
  target_compile_options(morph_bridge_tests PRIVATE /utf-8 /W4 /permissive-)
  target_link_libraries(morph_bridge_tests PRIVATE morph_bridge_core)
  set_target_properties(morph_bridge_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)
  add_test(NAME morph_bridge_tests COMMAND morph_bridge_tests)
endif()
