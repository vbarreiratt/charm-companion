cmake_minimum_required(VERSION 3.14)
project(charm_companion_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
)

add_executable(test_event_bus
    test_event_bus.cpp
    ../src/shell/event_bus.cpp
)
target_link_libraries(test_event_bus gtest)
gtest_discover_tests(test_event_bus)
