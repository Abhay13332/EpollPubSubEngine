
add_executable(FileEventSystem src/filepubsub.cpp)
add_executable(PubSub src/pubsub.cpp)

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

   
