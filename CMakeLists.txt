cmake_minimum_required(VERSION 4.3)

project (FileEventSystem CXX C)
include(FetchContent)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GLOBAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/includes")
add_subdirectory(server)
add_subdirectory(sdk)
add_subdirectory(examples)