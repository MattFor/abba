cmake_minimum_required(VERSION 4.2)

project(abba)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared code
add_library(abba_core
        src/settings/Settings.cpp
        src/utilities/FileOperations.cpp
        src/utilities/Hashing.cpp
        src/modules/memory/MemoryScanner.cpp
        include/modules/memory/MemoryScanner.h
        src/modules/memory/maps/MemoryMap.cpp
        include/modules/memory/maps/MemoryMap.h
        src/modules/memory/maps/LinuxMemory.cpp
        include/modules/memory/maps/LinuxMemory.h
        src/modules/memory/maps/WindowsMemory.cpp
        include/modules/memory/maps/WindowsMemory.h
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
        stdc++exp
)

target_compile_definitions(abba PRIVATE
        $<$<CONFIG:Debug>:LOG=1>
        $<$<NOT:$<CONFIG:Debug>>:LOG=0>
)

# Tests
enable_testing()

add_executable(abba_tests
        tests/main.cpp
        tests/suite/SettingsTests.cpp
        tests/suite/HashingTests.cpp
        tests/helpers/TempFile.h
        tests/helpers/TestRunner.h
        tests/suite/MemoryScannerTests.cpp
)

target_link_libraries(abba_tests
        PRIVATE
        abba_core
        stdc++exp
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