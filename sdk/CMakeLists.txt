cmake_minimum_required(VERSION 4.1)

project(PUBSUBSDK VERSION 1.0 LANGUAGES CXX)

add_library(pubSubSDK STATIC src/sdklib_root.cpp)
target_include_directories(pubSubSDK PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/includes>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../core/includes>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../external/includes>
    $<INSTALL_INTERFACE:include>
)
include(FetchContent)
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        v0.9.5 # Use the latest version
    )
FetchContent_MakeAvailable(magic_enum)
target_link_libraries(pubSubSDK PRIVATE
 $<BUILD_INTERFACE:magic_enum::magic_enum>
    $<INSTALL_INTERFACE:magic_enum::magic_enum>
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
install(TARGETS pubSubSDK EXPORT pubsubSDKTargets ARCHIVE DESTINATION lib)
install(EXPORT pubsubSDKTargets FILE pubsubSDKConfig.cmake NAMESPACE pubSubSDK:: DESTINATION lib/cmake/pubSubSDK)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY ../core/includes/ DESTINATION include)
 
endif()