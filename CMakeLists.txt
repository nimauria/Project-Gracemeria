cmake_minimum_required(VERSION 3.25)
project(ProjectGracemeria VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GRACEMERIA_BUILD_TESTS "Build Project Gracemeria tests" ON)
set(XENON_RECOMP_ROOT "" CACHE PATH "Path to the authoritative Xenon-Recomp source tree")
if(NOT XENON_RECOMP_ROOT OR NOT EXISTS "${XENON_RECOMP_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "Set XENON_RECOMP_ROOT to the current Xenon-Recomp source tree")
endif()

set(XENON_DEPENDENCY_MODE SYSTEM CACHE STRING "" FORCE)
set(XENON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(XENON_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(XENON_BUILD_LAUNCHER OFF CACHE BOOL "" FORCE)
set(XENON_ENABLE_GRAPHICS OFF CACHE BOOL "" FORCE)
set(XENON_ENABLE_INPUT ON CACHE BOOL "" FORCE)
set(XENON_ENABLE_AUDIO OFF CACHE BOOL "" FORCE)
add_subdirectory("${XENON_RECOMP_ROOT}" "${CMAKE_BINARY_DIR}/xenon-recomp" EXCLUDE_FROM_ALL)

add_library(gracemeria_analysis STATIC
  src/analysis/hint_provider.cpp
  src/analysis/ac6_migration.cpp
)
target_include_directories(gracemeria_analysis PUBLIC include)
target_link_libraries(gracemeria_analysis PUBLIC Xenon::Recomp)
# Default source for Ac6HintProvider::default_revisions_root() (see
# include/gracemeria/hint_provider.hpp) - this source tree's own revisions/
# directory. Packaging Gracemeria as an installed module later replaces this
# compiled-in default with the installed package root; explicit constructor
# callers (tests) are unaffected either way.
target_compile_definitions(gracemeria_analysis PRIVATE
  GRACEMERIA_REVISIONS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/revisions")

add_executable(gracemeria-recomp src/tools/gracemeria_recomp.cpp)
target_link_libraries(gracemeria-recomp PRIVATE gracemeria_analysis Xenon::Recomp)

# Title-specific AC6_recomp -> Xenon Analysis Hint Schema V2 migration tool
# (development tooling only; Xenon's own runtime/driver never reads ReXGlue
# TOML - see include/gracemeria/ac6_migration.hpp).
add_executable(migrate-ac6-recomp src/tools/migrate_ac6_recomp.cpp)
target_link_libraries(migrate-ac6-recomp PRIVATE gracemeria_analysis Xenon::Recomp)

# Import coverage intentionally uses the real XenonSession/ExportRegistry so
# its registered/stub/missing result follows the current runtime rather than a
# duplicated Gracemeria list.
add_executable(gracemeria-import-coverage src/tools/import_coverage.cpp)
target_link_libraries(gracemeria-import-coverage PRIVATE Xenon::Core)

if(GRACEMERIA_BUILD_TESTS)
  enable_testing()
  add_executable(gracemeria_hint_provider_tests tests/hint_provider_tests.cpp)
  target_link_libraries(gracemeria_hint_provider_tests PRIVATE gracemeria_analysis)
  add_test(NAME gracemeria_hint_provider_tests COMMAND gracemeria_hint_provider_tests)

  add_executable(gracemeria_ac6_migration_tests tests/ac6_migration_tests.cpp)
  target_link_libraries(gracemeria_ac6_migration_tests PRIVATE gracemeria_analysis)
  add_test(NAME gracemeria_ac6_migration_tests COMMAND gracemeria_ac6_migration_tests)

  add_library(gracemeria_abi_fixture SHARED tests/abi_fixture.cpp)
  target_include_directories(gracemeria_abi_fixture PRIVATE "${XENON_RECOMP_ROOT}/include")
  set_target_properties(gracemeria_abi_fixture PROPERTIES PREFIX "")

  add_executable(gracemeria_module_abi_tests tests/module_abi_tests.cpp)
  target_link_libraries(gracemeria_module_abi_tests PRIVATE Xenon::CPU ${CMAKE_DL_LIBS})
  add_dependencies(gracemeria_module_abi_tests gracemeria_abi_fixture)
  add_test(NAME gracemeria_module_abi_tests
           COMMAND gracemeria_module_abi_tests $<TARGET_FILE:gracemeria_abi_fixture>)

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME gracemeria_metadata_tests
           COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_metadata.py")
endif()
