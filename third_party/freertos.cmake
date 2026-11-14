include_guard(GLOBAL)

include(FetchContent)

FetchContent_Declare(
    freertos_kernel
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS
)

FetchContent_MakeAvailable(freertos_kernel)

FetchContent_GetProperties(freertos_kernel)

target_include_directories(freertos_kernel PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

get_target_property(FREERTOS_SOURCES freertos_kernel SOURCES)

# SOMA injects task.c and queue.c in its extensions so it can profile kernel
# task stacks and register every queue at the kernel creation/deletion hooks.
list(FILTER FREERTOS_SOURCES EXCLUDE REGEX ".*tasks.c")
list(FILTER FREERTOS_SOURCES EXCLUDE REGEX ".*queue.c")
set_property(TARGET freertos_kernel PROPERTY SOURCES ${FREERTOS_SOURCES})

target_compile_options(freertos_kernel_port PRIVATE
    -Wno-cast-function-type
)
