cmake_minimum_required(VERSION 4.2)

project(abba)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared code
add_library(abba_core
        src/settings/Settings.cpp
        src/utilities/FileOperations.cpp
)

target_include_directories(abba_core
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lib
)

target_compile_definitions(abba_core PRIVATE
        $<$<CONFIG:Debug>:LOG=1>
        $<$<NOT:$<CONFIG:Debug>>:LOG=0>
)

# Main program
add_executable(abba
        main.cpp
)

target_link_libraries(abba
        PRIVATE
        abba_core
)

target_compile_definitions(abba PRIVATE
        $<$<CONFIG:Debug>:LOG=1>
        $<$<NOT:$<CONFIG:Debug>>:LOG=0>
)

# Tests
enable_testing()

add_executable(abba_tests
        tests/main.cpp
        tests/SettingsTests.cpp
)

target_link_libraries(abba_tests
        PRIVATE
        abba_core
)

target_compile_definitions(abba_tests PRIVATE
        $<$<CONFIG:Debug>:LOG=1>
        $<$<NOT:$<CONFIG:Debug>>:LOG=0>
)

add_test(
        NAME abba_tests
        COMMAND abba_tests
)

set_tests_properties(abba_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)