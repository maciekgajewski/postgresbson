project(postgresbson)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Release)

add_subdirectory(pgbson)
add_subdirectory(tools)
add_subdirectory(test)
