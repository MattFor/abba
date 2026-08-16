cmake_minimum_required(VERSION 4.2)
project(abba)

set(CMAKE_CXX_STANDARD 26)

add_executable(abba main.cpp
        src/settings/Settings.cpp
        include/settings/Settings.h
        src/utilities/FileOperations.cpp
        include/utilities/FileOperations.h
        include/utilities/VectorHelpers.h
        include/utilities/Logger.h)

target_compile_definitions(abba PRIVATE
        $<$<CONFIG:Debug>:LOG=1>
        $<$<NOT:$<CONFIG:Debug>>:LOG=0>
)