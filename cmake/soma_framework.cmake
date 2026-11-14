cmake_minimum_required(VERSION 3.23.0)

include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET soma_config)
    message(FATAL_ERROR "SOMA requires an INTERFACE target named 'soma_config' that provides soma_config.h.")
endif()

# FreeRTOS requires a target with this exact name. SOMA owns it so every build
# passes through SOMA's FreeRTOSConfig.h composition layer.
if(TARGET freertos_config)
    message(FATAL_ERROR "SOMA owns the 'freertos_config' target; provide all settings through 'soma_config'.")
endif()

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config BEFORE INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/../freertos_config
    ${CMAKE_CURRENT_LIST_DIR}/../include
)
target_link_libraries(freertos_config INTERFACE soma_config)

# FreeRTOS source-port selection is set by the consumer before adding SOMA.
if(NOT FREERTOS_PORT OR NOT FREERTOS_PORT MATCHES "^[A-Za-z0-9_]+$")
    message(FATAL_ERROR "Set FREERTOS_PORT to a FreeRTOS port name, for example GCC_ARM_CM4F or MSVC_MINGW.")
endif()

# SOMA's heap-region API is implemented against FreeRTOS heap_5.
set(FREERTOS_HEAP "5" CACHE STRING "FreeRTOS heap implementation selected by SOMA" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/../third_party/freertos.cmake)

add_library(freertos_extension STATIC)

set_target_properties(freertos_extension PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

file(GLOB_RECURSE LIB_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../src/*.c" "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp")
list(FILTER LIB_SOURCES INCLUDE REGEX "freertos_.*\\..*")

target_sources(freertos_extension PRIVATE
    ${LIB_SOURCES}
)

target_link_libraries(freertos_extension PRIVATE
    freertos_kernel
    freertos_kernel_include
    freertos_kernel_port
    soma_framework
)

target_link_libraries(freertos_extension PUBLIC
   freertos_kernel_port
)

# search for all library source files
file(GLOB_RECURSE LIB_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../src/*.c" "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX "freertos_.*\\..*")
add_library(soma_framework STATIC)
add_library(SOMA::soma ALIAS soma_framework)

set_target_properties(soma_framework PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_sources(soma_framework PRIVATE
    ${LIB_SOURCES}
)

target_compile_options(soma_framework PRIVATE
    -Werror
    -Wall
    -Wextra
    -Wno-pedantic
)

target_include_directories(soma_framework PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../third_party
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(soma_framework PUBLIC
    soma_config
    freertos_kernel_include
    freertos_extension
)
