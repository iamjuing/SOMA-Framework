cmake_minimum_required(VERSION 3.23.0)

# The reusable support required by SOMA's host-side unit tests. This is an
# object library so its runner and host-port objects precede SOMA's static
# libraries on every consumer's link line.
#
if(FREERTOS_PORT AND NOT FREERTOS_PORT STREQUAL "MSVC_MINGW")
    message(FATAL_ERROR "The SOMA Windows test fixture requires FREERTOS_PORT=MSVC_MINGW")
endif()
if(TARGET soma_framework AND NOT FREERTOS_PORT STREQUAL "MSVC_MINGW")
    message(FATAL_ERROR "Configure SOMA with FREERTOS_PORT=MSVC_MINGW before loading its Windows test fixture")
endif()

if(NOT TARGET soma_framework)
    add_library(soma_config INTERFACE)
    target_include_directories(soma_config INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
    set(FREERTOS_PORT MSVC_MINGW)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/soma_framework.cmake")
endif()

# GoogleTest and GoogleMock are test-fixture dependencies only.
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/googletest.cmake)

add_library(soma_test_fixture OBJECT)

target_sources(soma_test_fixture PRIVATE
    main.cpp
    test_common.cpp
    windows_port.cpp
)

target_include_directories(soma_test_fixture PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# test_common.h exposes both the SOMA and GoogleTest types to test sources.
target_link_libraries(soma_test_fixture PUBLIC
    soma_framework
    GTest::gtest
    GTest::gmock
)
