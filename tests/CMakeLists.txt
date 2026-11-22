add_executable(filon-tests test_random.cpp)
target_link_libraries(filon-tests PRIVATE filon-core)
add_test(NAME parite-hasard COMMAND filon-tests)

add_executable(filon-test-world test_world.cpp)
target_link_libraries(filon-test-world PRIVATE filon-core)
target_compile_definitions(filon-test-world PRIVATE FILON_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME parite-monde COMMAND filon-test-world)
