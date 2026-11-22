add_executable(filon-tests test_random.cpp)
target_link_libraries(filon-tests PRIVATE filon-core)
add_test(NAME parite-hasard COMMAND filon-tests)
