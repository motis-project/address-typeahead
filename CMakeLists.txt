project(address-typeahead)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # required for clang-format
set(compiler-flags "-std=c++14 -Wall -Wextra -Werror")


################################
# Dependencies
################################
find_package(ZLIB REQUIRED)

add_subdirectory(external_lib/googletest/googletest EXCLUDE_FROM_ALL)
add_subdirectory(external_lib/guess EXCLUDE_FROM_ALL)

include_directories(include)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(SYSTEM external_lib/googletest/googletest/include)
include_directories(SYSTEM external_lib/osmium/include)
include_directories(SYSTEM external_lib/guess/include)


################################
# Library
################################
file(GLOB_RECURSE address-typeahead-files src/*.cc)
add_library(address-typeahead ${address-typeahead-files})
target_link_libraries(address-typeahead ${ZLIB_LIBRARIES})
set_target_properties(address-typeahead PROPERTIES COMPILE_FLAGS ${compiler-flags})


################################
# Example Executable
################################
file(GLOB_RECURSE at-example-files example/*.cc)
add_executable(at-example ${at-example-files})
target_link_libraries(at-example address-typeahead guess)
set_target_properties(at-example PROPERTIES COMPILE_FLAGS ${compiler-flags})


################################
# Tests
################################
file(GLOB_RECURSE at-test-files test/*.cc)
add_executable(at-test ${at-test-files})
target_link_libraries(at-test address-typeahead gtest gtest_main)
set_target_properties(at-test PROPERTIES COMPILE_FLAGS ${compiler-flags})


################################
# clang-format check
################################
add_custom_target(format-check
  find src include test -type f -name "*.cc" -o -name "*.h" -print0 | xargs -0 clang-format -i
  COMMAND
  git status --porcelain | xargs -I {} -0 test -z \"{}\"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Checking source code formatting"
  VERBATIM
)


################################
# Lint
################################
add_custom_target(lint
  /usr/bin/python
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-clang-tidy.py
      -exit-on-error
      -header-filter "^${CMAKE_CURRENT_SOURCE_DIR}(include/)"
      "^${CMAKE_CURRENT_SOURCE_DIR}/include/"
      "^${CMAKE_CURRENT_SOURCE_DIR}/src/"
      "^${CMAKE_CURRENT_SOURCE_DIR}/test/"
  COMMENT "Running clang-tidy"
  VERBATIM
)
add_dependencies(lint address-typeahead)
