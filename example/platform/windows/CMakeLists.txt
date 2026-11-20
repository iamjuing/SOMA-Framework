cmake_minimum_required(VERSION 3.23)

project(soma_firmware LANGUAGES C CXX)

get_filename_component(EXAMPLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if(NOT WIN32 OR NOT MINGW)
    message(FATAL_ERROR "The Windows reference firmware requires a Windows MinGW toolchain.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(soma_config INTERFACE)
target_include_directories(soma_config INTERFACE "${CMAKE_CURRENT_LIST_DIR}")

set(FREERTOS_PORT MSVC_MINGW)
add_subdirectory("${EXAMPLE_ROOT}/.." soma)

add_subdirectory("${EXAMPLE_ROOT}/src" application)

add_executable(soma_example
    console_input_stream.cpp
    console_output_stream.cpp
    main.cpp
    windows_host.cpp
)

target_link_libraries(soma_example PRIVATE soma_example_application)
target_compile_definitions(soma_example PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(soma_example PRIVATE -Wall -Wextra -Werror)

set_target_properties(soma_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

