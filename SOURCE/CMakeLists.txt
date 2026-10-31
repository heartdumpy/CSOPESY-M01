
cmake_minimum_required(VERSION 3.16)

project(CSOPESY_MO1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files currently in the project.
set(CSOPESY_SOURCES
    main.cpp
    ConsoleManager.cpp
    CommandInterpreter.cpp
    ScreenManager.cpp
    config.cpp
    ProcessManager.cpp
    Scheduler.cpp
    ReportManager.cpp
)

add_executable(CSOPESY ${CSOPESY_SOURCES})

# Make config.txt available in the build directory.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.txt
    ${CMAKE_CURRENT_BINARY_DIR}/config.txt
    COPYONLY
)

# Enable compiler warnings.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CSOPESY PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
