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

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(GLOBAL_INCLUDE_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR}/core/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/external/includes
    )
    enable_testing()
    include(GoogleTest)
    add_subdirectory(server)
    add_subdirectory(examples)
    add_subdirectory(tests)

endif()