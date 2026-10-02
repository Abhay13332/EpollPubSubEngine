cmake_minimum_required(VERSION 4.1)
project(PUBSUBSDKEXA VERSION 1.0 LANGUAGES CXX )
include(FetchContent)
FetchContent_Declare(
    
)
function(add_my_app target_name source_file)
add_executable(${target_name} ${source_file})
    
    target_link_libraries(${target_name} PRIVATE common_lib)
endfunction()
