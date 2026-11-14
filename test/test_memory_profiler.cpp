// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "gtest/gtest.h"

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/text/string.h"
#include "soma/diagnostics/memory_profiler.h"
#include "soma/runtime/heap.h"
#include "soma/text/text_string_output.h"

using namespace SOMA;

namespace
{

bool contains( String_view text,
               String_view expected )
{
    size_t position = 0u;

    return text.find( expected, position );
}

void dormant_task( [[maybe_unused]] void* parameter )
{
    vTaskDelay( portMAX_DELAY );
}

} // namespace

TEST( memory_profiler, renders_active_task_stack_and_excludes_deleted_task )
{
    constexpr char c_task_name[] = "memory_task";
    TaskHandle_t task_handle = nullptr;
    Memory_profiler profiler;
    String::Length<4096u> output_text;
    Text_string_output output { output_text };

    ASSERT_EQ( xTaskCreate( dormant_task,
                            c_task_name,
                            configMINIMAL_STACK_SIZE,
                            nullptr,
                            1u,
                            &task_handle ),
               pdPASS );

    profiler.to_text( output );

    String::Length<32u> heap_capacity_text;
    Text_string_output heap_capacity_output { heap_capacity_text };
    heap_capacity_output << bytes( Heap::statistics().capacity_B, 1u );

    EXPECT_TRUE( contains( output_text, heap_capacity_text ) );
    EXPECT_TRUE( contains( output_text, c_task_name ) );

    vTaskDelete( task_handle );

    output_text.clear();
    profiler.to_text( output );

    EXPECT_FALSE( contains( output_text, c_task_name ) );
}
