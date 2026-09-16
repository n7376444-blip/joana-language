cmake_minimum_required(VERSION 3.20)
project(joana-language VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(joana STATIC
    src/joana.cpp
)

target_include_directories(joana
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(joana_cli
    src/main.cpp
)

target_link_libraries(joana_cli PRIVATE joana)
set_target_properties(joana_cli PROPERTIES OUTPUT_NAME joana)

enable_testing()
add_executable(joana_tests
    tests/test_main.cpp
)

target_link_libraries(joana_tests PRIVATE joana)
add_test(NAME joana_tests COMMAND joana_tests)
