cmake_minimum_required(VERSION 3.16)

project(
    CSOPESY_MO1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(
    CORE_SOURCES
    Process.cpp
    ProcessManager.cpp
    Instruction.cpp
    InstructionExecutor.cpp
    InstructionGenerator.cpp
    Scheduler.cpp
    config.cpp
    ReportManager.cpp
)

add_library(
    emulator_core
    ${CORE_SOURCES}
)

target_include_directories(
    emulator_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    emulator_core
    PUBLIC
    Threads::Threads
)

add_executable(
    CSOPESY
    main.cpp
    ConsoleManager.cpp
    CommandInterpreter.cpp
    ScreenManager.cpp
)

target_link_libraries(
    CSOPESY
    PRIVATE
    emulator_core
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.txt
    ${CMAKE_CURRENT_BINARY_DIR}/config.txt
    COPYONLY
)

# Copies config beside the executable, including Visual Studio builds.
add_custom_command(
    TARGET CSOPESY
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config.txt
        $<TARGET_FILE_DIR:CSOPESY>/config.txt
)

foreach(target emulator_core CSOPESY)
    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE
            /W4
        )
    else()
        target_compile_options(
            ${target}
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endforeach()

option(
    BUILD_TESTING
    "Build the separate regression test executable"
    OFF
)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        member2_tests
        tests/member2_tests.cpp
    )

    target_link_libraries(
        member2_tests
        PRIVATE
        emulator_core
    )

    add_test(
        NAME member2_tests
        COMMAND member2_tests
    )
endif()
