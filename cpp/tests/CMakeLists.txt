cmake_minimum_required(VERSION 3.20)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test executable
add_executable(unit_tests
    test_mesh.cpp
    test_solver.cpp
    test_types.cpp
)

target_link_libraries(unit_tests
    PRIVATE
    swe_lib
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
