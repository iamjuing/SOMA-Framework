cmake_minimum_required(VERSION 3.23.0)

project(SOMA LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(test)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/soma_framework.cmake)
endif()
