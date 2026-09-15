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
if (WIN32)
    # ugly, but for now live with it, because i am lazy and
    # this allows me to just run abba_process_tests and not worry about anything else
    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)

    add_library(abba_hook SHARED
            src/modules/network/HookDll.cpp
    )
    set_target_properties(abba_hook PROPERTIES PREFIX "")

    set_target_properties(abba_hook PROPERTIES FOLDER "Internal")

    target_include_directories(abba_hook PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(abba_hook PRIVATE abba_core ws2_32 stdc++exp)

    target_compile_definitions(abba_hook PRIVATE
            $<$<CONFIG:Debug>:LOG=1>
            $<$<NOT:$<CONFIG:Debug>>:LOG=0>
    )
endif ()
if (WIN32)
    add_executable(abba_socket_target
            tests/helpers/SocketTarget.cpp
    )

    target_link_libraries(abba_socket_target
            PRIVATE
            ws2_32
            stdc++exp
    )

    set_target_properties(abba_socket_target PROPERTIES FOLDER "Internal")

endif ()

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

if (WIN32)

    # Network Manager tests
    add_executable(abba_process_tests
            tests/main_process.cpp
            tests/suite/NetworkTests.cpp
            tests/suite/IntegrationTests.cpp
            # or wherever you put the self-attach test
    )

    target_link_libraries(abba_process_tests PRIVATE abba_core stdc++exp)

    target_compile_definitions(abba_process_tests PRIVATE
            $<$<CONFIG:Debug>:LOG=1>
            $<$<NOT:$<CONFIG:Debug>>:LOG=0>
    )

    add_dependencies(abba_process_tests abba_hook abba_socket_target)

    add_custom_command(TARGET abba_process_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:abba_hook>
            $<TARGET_FILE_DIR:abba_process_tests>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:abba_socket_target>
            $<TARGET_FILE_DIR:abba_process_tests>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MINGW_BIN_DIR}/libwinpthread-1.dll
            $<TARGET_FILE_DIR:abba_process_tests>
    )

    add_test(NAME abba_process_tests COMMAND abba_process_tests)

    # Integration tests

    # Network scanning and Memory scanning together

endif ()