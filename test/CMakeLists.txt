add_subdirectory( ${CMAKE_CURRENT_LIST_DIR}/../test_fixture ${CMAKE_CURRENT_BINARY_DIR}/test_fixture )

file(GLOB SOMA_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_*.cpp")
list(FILTER SOMA_TEST_SOURCES EXCLUDE REGEX "/test_common\\.cpp$")

add_executable(soma_tests ${SOMA_TEST_SOURCES})

set_target_properties(soma_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(soma_tests PRIVATE
    soma_test_fixture
)

target_compile_options(soma_tests PRIVATE
    -g
    -Og
)

add_test(NAME soma_tests COMMAND soma_tests)
