add_library(soma_example_application STATIC
    active_components/blinky.cpp
    main.cpp
)

target_include_directories(soma_example_application PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/.."
)

target_link_libraries(soma_example_application PUBLIC
    SOMA::soma
)
