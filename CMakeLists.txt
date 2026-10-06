cmake_minimum_required(VERSION 4.3)

project (EPOLLPUBSUB CXX C)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_subdirectory(sdk)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG main
    )
    FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        v0.9.5
    )

    FetchContent_MakeAvailable(magic_enum)
    set("MAGIC_ENUM_LIBRARIES" magic_enum::magic_enum)
    FetchContent_MakeAvailable(googletest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    set(GLOBAL_INCLUDE_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR}/core/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/external/includes
    )
    include(GoogleTest)
    enable_testing()
    add_subdirectory(examples)
    add_subdirectory(server)
    add_subdirectory(tests)

    
endif()