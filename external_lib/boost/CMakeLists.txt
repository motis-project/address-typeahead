project(boost_serialization)
cmake_minimum_required(VERSION 2.8)

include_directories(include)
file(GLOB_RECURSE src-files libs/serialization/src/*.cpp)
add_library(boost_serialization STATIC ${src-files})
