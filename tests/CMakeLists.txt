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

add_executable(test_personality_core
    test_personality_core.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
)
target_link_libraries(test_personality_core gtest gtest_main)
gtest_discover_tests(test_personality_core)

add_executable(test_personality_api
    test_personality_api.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
)
target_link_libraries(test_personality_api gtest gtest_main)
gtest_discover_tests(test_personality_api)

