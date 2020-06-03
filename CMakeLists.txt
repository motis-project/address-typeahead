cmake_minimum_required(VERSION 3.16)
project(address-typeahead)

option(AT_LINT "Run clang-tidy with the compiler." OFF)
if(AT_LINT)
  include(cmake/clang-tidy.cmake)
endif()

if (NOT MSVC)
  set(compiler-flags "-Wall -Werror -Wextra")
else()
  set(compiler-flags "-DNOMINMAX -DXML_STATIC")
endif()

################################
# Boost
################################
if (MSVC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()
find_package(Boost REQUIRED)

link_directories(${Boost_LIBRARY_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})


################################
# Dependencies
################################
find_package(Threads)
include(cmake/pkg.cmake)


################################
# Linter
################################
option(ADDRESS_TYPEAHEAD_LINT "Run clang-tidy with the compiler." OFF)
if(ADDRESS_TYPEAHEAD_LINT)
  # clang-tidy will be run on all targets defined hereafter
  include(cmake/clang-tidy.cmake)
endif()


################################
# Library
################################
file(GLOB_RECURSE address-typeahead-files src/*.cc)
add_library(address-typeahead EXCLUDE_FROM_ALL ${address-typeahead-files})
target_compile_features(address-typeahead PUBLIC cxx_std_17)
target_compile_definitions(address-typeahead PUBLIC NOMINMAX)
if (MSVC)
  target_link_libraries(address-typeahead ws2_32)
endif()
target_link_libraries(address-typeahead
  osmium
  protozero
  guess
  cereal
  zlibstatic
  expat
  utl
  ${CMAKE_THREAD_LIBS_INIT}
)
set_target_properties(address-typeahead PROPERTIES COMPILE_FLAGS ${compiler-flags})
target_include_directories(address-typeahead PUBLIC include)


################################
# Example Executable
################################
file(GLOB_RECURSE at-example-files example/*.cc)
add_executable(at-example EXCLUDE_FROM_ALL ${at-example-files})
target_link_libraries(at-example address-typeahead)
set_target_properties(at-example PROPERTIES COMPILE_FLAGS ${compiler-flags})


################################
# Tests
################################
file(GLOB_RECURSE at-test-files test/*.cc)
add_executable(at-test EXCLUDE_FROM_ALL ${at-test-files})
target_link_libraries(at-test address-typeahead gtest gtest_main)
set_target_properties(at-test PROPERTIES COMPILE_FLAGS ${compiler-flags})