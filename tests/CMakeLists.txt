add_executable(server_utest pubserver/test_main.cpp)

target_include_directories(server_utest PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${GLOBAL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../server/includes
)
target_link_libraries(server_utest PRIVATE
GTest::gtest_main
magic_enum::magic_enum
)
gtest_discover_tests(server_utest)

