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
        src/modules/network/PacketValidator.cpp
        include/modules/network/netcap/PacketValidator.h
        include/modules/network/netcap/hook/HookProtocol.h
        include/modules/network/netcap/PacketContext.h
        include/modules/network/netcap/IPacketValidator.h

)
if (WIN32)
    target_sources(abba_core PRIVATE
            include/modules/network/NetworkMonitor.h
            src/modules/network/NetworkMonitor.cpp
            src/utilities/Injector.cpp
            include/utilities/Injector.h
    )
endif ()

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


        include/modules/network/netcap/hook/HookProtocol.h
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
if(WIN32)
    add_library(abba_hook SHARED
            src/modules/network/HookDll.cpp
    )
    set_target_properties(abba_hook PROPERTIES PREFIX "")

    target_include_directories(abba_hook PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(abba_hook PRIVATE abba_core ws2_32 stdc++exp)

    target_compile_definitions(abba_hook PRIVATE
            $<$<CONFIG:Debug>:LOG=1>
            $<$<NOT:$<CONFIG:Debug>>:LOG=0>
    )
endif ()
if(WIN32)
    add_executable(abba_socket_target
            src/utilities/SocketTarget.cpp
    )
    target_link_libraries(abba_socket_target
            PRIVATE
            ws2_32
            stdc++exp
    )

endif()

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

if(WIN32)
    add_executable(abba_network_tests
            tests/main_network.cpp        # or wherever you put the self-attach test
    )

    target_link_libraries(abba_network_tests PRIVATE abba_core stdc++exp)

    target_compile_definitions(abba_network_tests PRIVATE
            $<$<CONFIG:Debug>:LOG=1>
            $<$<NOT:$<CONFIG:Debug>>:LOG=0>
    )

    add_dependencies(abba_network_tests abba_hook)

    add_custom_command(TARGET abba_network_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:abba_hook>
            $<TARGET_FILE_DIR:abba_network_tests>
    )

    add_test(NAME abba_network_tests COMMAND abba_network_tests)
endif()