cmake_minimum_required(VERSION 4.2)
project(abba)

set(CMAKE_CXX_STANDARD 26)

add_executable(abba main.cpp
        src/settings/Settings.cpp
        include/settings/Settings.h
        src/utilities/FileOperations.cpp
        include/utilities/FileOperations.h)
