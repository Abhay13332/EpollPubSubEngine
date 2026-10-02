FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        v0.9.5 # Use the latest version
    )
add_executable(FileEventSystem src/filepubsub.cpp)
add_executable(PubSub src/pubsub.cpp)
FetchContent_MakeAvailable(magic_enum)
set("MAGIC_ENUM_LIBRARIES" magic_enum::magic_enum)
target_include_directories(
    FileEventSystem PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
        ${GLOBAL_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}

)
target_link_libraries(
    FileEventSystem PRIVATE       
        ${MAGIC_ENUM_LIBRARIES}

   )
target_include_directories(
    PubSub PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
        ${GLOBAL_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}

)
target_link_libraries(
    PubSub PRIVATE       
        ${MAGIC_ENUM_LIBRARIES}
   )

   
