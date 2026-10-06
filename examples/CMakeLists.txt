cmake_minimum_required(VERSION 4.1)
project(PUBSUBSDKEXA VERSION 1.0 LANGUAGES CXX )
include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(add_my_app target_name source_file)
add_executable(${target_name} ${source_file})

target_link_libraries(${target_name} PRIVATE pubSubSDK)
endfunction()


add_my_app(pub src/pubInt.cpp)
add_my_app(sub src/subInt.cpp)
add_my_app(pubMult src/pubMultiplex.cpp)
add_my_app(subMult src/subMultiplex.cpp)
