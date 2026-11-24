add_executable(alpha_tests
  test_main.cpp
  core/version_test.cpp)
target_link_libraries(alpha_tests PRIVATE alpha_core doctest::doctest)
target_compile_definitions(alpha_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
alpha_set_warnings(alpha_tests)
add_test(NAME alpha_tests COMMAND alpha_tests)
