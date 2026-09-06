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

add_executable(test_home_app
    test_home_app.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
    ../src/apps/app_base.cpp
    ../src/apps/home/home_app.cpp
    ../src/apps/home/home_ui.cpp
)
target_link_libraries(test_home_app gtest gtest_main)
gtest_discover_tests(test_home_app)

add_executable(test_scenes_app
    test_scenes_app.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
    ../src/apps/app_base.cpp
    ../src/apps/scenes/scene_registry.cpp
    ../src/apps/scenes/scenes_app.cpp
)
target_link_libraries(test_scenes_app gtest gtest_main)
gtest_discover_tests(test_scenes_app)

add_executable(test_shell_integration
    test_shell_integration.cpp
    ../src/utils/canvas_wrapper.cpp
    ../src/shell/event_bus.cpp
    ../src/shell/shell.cpp
    ../src/shell/hal/display_hal.cpp
    ../src/shell/hal/touch_hal.cpp
    ../src/shell/hal/imu_hal.cpp
    ../src/shell/hal/power_hal.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
    ../src/apps/app_base.cpp
    ../src/apps/home/home_app.cpp
    ../src/apps/home/home_ui.cpp
    ../src/apps/scenes/scene_registry.cpp
    ../src/apps/scenes/scenes_app.cpp
    ../src/apps/scenes/planet_scene.cpp
    ../src/apps/scenes/eye_scene.cpp
)
target_link_libraries(test_shell_integration gtest gtest_main)
gtest_discover_tests(test_shell_integration)

add_executable(test_canvas_wrapper
    test_canvas_wrapper.cpp
    ../src/utils/canvas_wrapper.cpp
)
target_link_libraries(test_canvas_wrapper gtest gtest_main)
gtest_discover_tests(test_canvas_wrapper)

