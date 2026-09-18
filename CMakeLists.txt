cmake_minimum_required(VERSION 3.16)
project(flyreflex C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(flyreflex_core
  app/flyreflex/src/looming_input.c
  app/flyreflex/src/reflex_engine.c
  app/flyreflex/src/safety_arbiter.c
  app/flyreflex/src/flyreflex_runtime.c)
target_include_directories(flyreflex_core PUBLIC app/flyreflex/include)
target_compile_definitions(flyreflex_core PUBLIC _POSIX_C_SOURCE=200809L)
target_compile_options(flyreflex_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(flyreflex_host app/flyreflex/src/flyreflex_main.c)
target_link_libraries(flyreflex_host PRIVATE flyreflex_core)
target_compile_options(flyreflex_host PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_executable(test_flyreflex tests/test_flyreflex.c)
target_link_libraries(test_flyreflex PRIVATE flyreflex_core)
target_compile_options(test_flyreflex PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME flyreflex_core_tests COMMAND test_flyreflex)

