add_executable(plexa_tests test_plexa.cpp)
add_dependencies(plexa_tests PlExA)
target_link_libraries(plexa_tests PRIVATE PlExA)
setup_sanitizers(plexa_tests)

add_test(NAME PlExATest COMMAND plexa_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})