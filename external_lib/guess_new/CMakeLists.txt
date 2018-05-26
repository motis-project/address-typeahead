cmake_minimum_required(VERSION 2.6)
project(guess)

add_library(guess src/guesser.cc)
set_target_properties(guess PROPERTIES COMPILE_FLAGS "-std=c++11")

include_directories(include)
add_executable(guess-example EXCLUDE_FROM_ALL example/main.cc)
target_link_libraries(guess-example guess)
set_target_properties(guess-example PROPERTIES COMPILE_FLAGS "-std=c++11")
