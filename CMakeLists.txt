cmake_minimum_required(VERSION 3.0)
project(address-typeahead)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # required for clang-tidy

if (NOT MSVC)
  set(compiler-flags "-std=c++14 -Wall -Werror -Wextra")
else()
  set(compiler-flags "-DNOMINMAX -DXML_STATIC")
endif()


################################
# 3rd Party (Boost, pthread)
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

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deps")
  add_subdirectory(deps)
endif()
add_subdirectory(external_lib/cereal EXCLUDE_FROM_ALL)
add_subdirectory(external_lib/guess EXCLUDE_FROM_ALL)


################################
# Library
################################
file(GLOB_RECURSE address-typeahead-files src/*.cc)
add_library(address-typeahead EXCLUDE_FROM_ALL ${address-typeahead-files})
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
target_include_directories(at-test PRIVATE SYSTEM external_lib/googletest/googletest/include)


################################
# clang-format check
################################
add_custom_target(address-typeahead-format-check
  find src include test example -type f -name "*.cc" -o -name "*.h" -print0 | xargs -0 clang-format -i
  COMMAND
  git status --porcelain | xargs -I {} -0 test -z \"{}\"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Checking source code formatting"
  VERBATIM
)


################################
# Lint
################################
add_custom_target(address-typeahead-lint
  /usr/bin/python
  ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-clang-tidy.py
    -exit-on-error
      -quiet
      -clang-tidy-binary clang-tidy-6.0
    -header-filter "^${CMAKE_CURRENT_SOURCE_DIR}(include/)"
    "^${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "^${CMAKE_CURRENT_SOURCE_DIR}/src/"
    "^${CMAKE_CURRENT_SOURCE_DIR}/test/"
  COMMENT "Running clang-tidy"
  VERBATIM
)
add_dependencies(address-typeahead-lint address-typeahead at-test at-example)
