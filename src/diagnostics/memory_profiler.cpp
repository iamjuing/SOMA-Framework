// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <limits>
#include <cinttypes>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/diagnostics/memory_profiler.h"
#include "soma/runtime/heap.h"
#include "soma/text/text_output.h"
#include "soma/text/text_string_output.h"
#include "soma/containers/i_queue_profile.h"
#include "soma/text/string_table_generator.h"
#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0
#include "../runtime/freertos_queue_profile.h"
#endif

namespace SOMA
{
    extern "C"
    {
        BaseType_t xTaskGetNextStackProfile( TaskStatus_t* p_task_status,
                                             char task_name[configMAX_TASK_NAME_LEN],
                                             configSTACK_DEPTH_TYPE* p_stack_depth,
                                             UBaseType_t* p_last_task_number,
                                             BaseType_t* p_has_last_task_number );
    }

    const String_table_generator::Column_t Memory_profiler::mc_heap_table_columns[mc_heap_table_column_count] = {
        { "Heap Size", 10, String_table_generator::Cell_alignment_e::right },
        { "Used", 10, String_table_generator::Cell_alignment_e::right },
        { "Peak", 10, String_table_generator::Cell_alignment_e::right },
        { "Peak Usage [%]", 9, String_table_generator::Cell_alignment_e::right } };

    const String_table_generator::Layout_t Memory_profiler::mc_heap_table_layout = {
        " ", '*', Array( mc_heap_table_columns ) };

    const String_table_generator::Column_t Memory_profiler::mc_task_table_columns[mc_task_table_column_count] = {
        { "Task Name", configMAX_TASK_NAME_LEN - 1u, String_table_generator::Cell_alignment_e::left },
        { "Stack Size [kB]", 10, String_table_generator::Cell_alignment_e::right },
        { "Peak Usage [%]", 9, String_table_generator::Cell_alignment_e::right } };

    const String_table_generator::Layout_t Memory_profiler::mc_task_table_layout = {
        " ", '*', Array( mc_task_table_columns ) };

    const String_table_generator::Column_t Memory_profiler::mc_queue_table_columns[mc_queue_table_column_count] = {
        { "Queue Name", 25, String_table_generator::Cell_alignment_e::left },
        { "Item Size", 8, String_table_generator::Cell_alignment_e::right },
        { "Capacity [items]", 8, String_table_generator::Cell_alignment_e::right },
        { "Size", 10, String_table_generator::Cell_alignment_e::right },
        { "Peak Usage [%]", 9, String_table_generator::Cell_alignment_e::right } };

    const String_table_generator::Layout_t Memory_profiler::mc_queue_table_layout = {
        " ", '*', Array( mc_queue_table_columns ) };

#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0
    const String_table_generator::Column_t Memory_profiler::mc_freertos_queue_table_columns[mc_freertos_queue_table_column_count]
        = {
        { "Queue Name", 25, String_table_generator::Cell_alignment_e::left },
        { "Item Size", 8, String_table_generator::Cell_alignment_e::right },
        { "Capacity [items]", 8, String_table_generator::Cell_alignment_e::right },
        { "Used", 8, String_table_generator::Cell_alignment_e::right },
        { "Size", 10, String_table_generator::Cell_alignment_e::right },
        { "Usage [%]", 9, String_table_generator::Cell_alignment_e::right } };

    const String_table_generator::Layout_t Memory_profiler::mc_freertos_queue_table_layout = {
        " ", '*', Array( mc_freertos_queue_table_columns ) };
#endif

    String_view Memory_profiler::byte_count_str( size_t size_B,
                                                 String& out_string ) const
    {
        out_string.clear();

        Text_string_output output { out_string };

        output << bytes( size_B, 1u );

        return out_string.view();
    }

    String_view Memory_profiler::percentage_str( size_t value,
                                                 size_t capacity,
                                                 String& out_string ) const
    {
        out_string.clear();

        out_string.fraction( static_cast<uint32_t>( value ) * 100u,
                             static_cast<uint32_t>( capacity ),
                             "%" PRIu32,
                             1u );

        return out_string.view();
    }

    String_view Memory_profiler::stack_size_str( size_t size_B,
                                                 String& out_string ) const
    {
        constexpr uint32_t c_kilobyte_B = 1024u;
        constexpr size_t c_decimal_digit_cnt = 3u;

        out_string.clear();
        out_string.fraction( static_cast<uint32_t>( size_B ),
                             c_kilobyte_B,
                             "%" PRIu32,
                             c_decimal_digit_cnt );

        return out_string.view();
    }

    String_view Memory_profiler::stack_peak_str( size_t stack_depth_words,
                                                 size_t stack_high_water_mark_words,
                                                 String& out_string ) const
    {
        out_string.clear();

        if( ( stack_depth_words > 0u ) &&
            ( stack_high_water_mark_words <= stack_depth_words ) )
        {
            const uint32_t stack_peak_used_words = static_cast<uint32_t>(
                stack_depth_words - stack_high_water_mark_words );

            out_string.fraction( stack_peak_used_words * 100u,
                                 static_cast<uint32_t>( stack_depth_words ),
                                 "%" PRIu32,
                                 1u );
        }
        else
        {
            out_string += "--.-";
        }

        return out_string.view();
    }

    void Memory_profiler::generate_heap_profile( Text_output& output )
    {
        const Heap::Statistics_t heap = Heap::statistics();

        if( heap.enabled == false )
        {
            output << "Heap disabled";

            return;
        }

        const size_t c_heap_used_B = heap.capacity_B - heap.free_B;
        const size_t c_heap_peak_B = heap.capacity_B - heap.minimum_ever_free_B;
        String_table_generator table { mc_heap_table_layout, output };
        String::Length<20u> capacity_str;
        String::Length<20u> used_str;
        String::Length<20u> peak_str;
        String::Length<20u> peak_percentage_str;

        table.header();
        table << byte_count_str( heap.capacity_B, capacity_str )
              << byte_count_str( c_heap_used_B, used_str )
              << byte_count_str( c_heap_peak_B, peak_str )
              << percentage_str( c_heap_peak_B, heap.capacity_B, peak_percentage_str );
    }

    void Memory_profiler::generate_task_profile( Text_output& output )
    {
        String_table_generator table { mc_task_table_layout, output };
        TaskStatus_t task_status {};
        char task_name[configMAX_TASK_NAME_LEN] {};
        configSTACK_DEPTH_TYPE stack_depth_words = 0u;
        UBaseType_t last_task_number = 0u;
        BaseType_t has_last_task_number = pdFALSE;

        table.header();

        while( xTaskGetNextStackProfile( &task_status,
                                         task_name,
                                         &stack_depth_words,
                                         &last_task_number,
                                         &has_last_task_number ) == pdTRUE )
        {
            String::Length<20u> str;

            table << task_name
                  << stack_size_str( static_cast<size_t>( stack_depth_words ) * sizeof( StackType_t ), str )
                  << stack_peak_str( stack_depth_words, task_status.usStackHighWaterMark, str );
        }
    }

    void Memory_profiler::generate_queue_profile( Text_output& output )
    {
        String_table_generator table { mc_queue_table_layout, output };

        output << "SOMA queues" << EOL;
        table.header();

        for( const auto& queue: I_queue_profile::objects() )
        {
            String::Length<20> str;

            table << queue.name()
                  << byte_count_str( queue.item_size_B(), str )
                  << queue.capacity()
                  << byte_count_str( queue.item_size_B() * queue.capacity(), str )
                  << percentage_str( queue.peak_cnt(), queue.capacity(), str );
        }

#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0
        output << EOL << "FreeRTOS queues" << EOL;
        generate_freertos_queue_profile( output );
#endif
    }

#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0
    void Memory_profiler::generate_freertos_queue_profile( Text_output& output )
    {
        String_table_generator table { mc_freertos_queue_table_layout, output };
        SOMA_Queue_profile_t queue_profile {};
        uint32_t cursor = 0u;

        table.header();

        while( soma_queue_next_profile( &cursor, &queue_profile ) == pdTRUE )
        {
            String::Length<20u> item_size;
            String::Length<20u> data_size;
            String::Length<20u> usage;

            const size_t queue_data_size_B = static_cast<size_t>( queue_profile.item_size_B ) *
                                             static_cast<size_t>( queue_profile.length );

            table << ( queue_profile.name != NULL ? queue_profile.name : "" )
                  << byte_count_str( queue_profile.item_size_B, item_size )
                  << queue_profile.length
                  << queue_profile.messages_waiting
                  << byte_count_str( queue_data_size_B, data_size )
                  << percentage_str( queue_profile.messages_waiting, queue_profile.length, usage );
        }
    }
#endif

    void Memory_profiler::to_text( Text_output& output )
    {
        generate_heap_profile( output );

        output << EOL;

        generate_task_profile( output );

        output << EOL;

        generate_queue_profile( output );
    }
} // namespace SOMA
