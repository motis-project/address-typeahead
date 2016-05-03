project(address-typeahead)
cmake_minimum_required(VERSION 2.8)

find_package(ZLIB REQUIRED)

include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(SYSTEM external_lib/osmium/include)

file(GLOB_RECURSE address-typeahead-files src/*.cc)
add_executable(address-typeahead ${address-typeahead-files})
target_link_libraries(address-typeahead ${ZLIB_LIBRARIES})

if (NOT MSVC)
  set_target_properties(address-typeahead PROPERTIES COMPILE_FLAGS "-std=c++14 -Wall -Wextra")
endif()
