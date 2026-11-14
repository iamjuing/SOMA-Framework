
include(FetchContent)

FetchContent_Declare( googletest
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../tools/googletest
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

target_compile_options( gtest PRIVATE
    -g
)
