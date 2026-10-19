cmake_minimum_required(VERSION 3.16)
project(aesterctl CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

set(CORE_SOURCES
    src/core/log.cpp
    src/core/sysfs.cpp
    src/core/state.cpp
    src/core/validate.cpp
)

set(CLI_SOURCES
    src/cli/args.cpp
    src/cli/commands.cpp
)

add_executable(aesterctl
    src/main.cpp
    ${CORE_SOURCES}
    ${CLI_SOURCES}
)

add_executable(test_validate
    tests/test_validate.cpp
    ${CORE_SOURCES}
)
