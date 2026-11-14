// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_MEMORY_PROFILER_H_
#define SOMA_MEMORY_PROFILER_H_

#include "soma/text/string_view.h"
#include "soma/text/string_table_generator.h"

namespace SOMA
{
    class Text_output;
    class I_queue_profile;
    class String;

    class Memory_profiler
    {
    public:
        void to_text( Text_output& output );

    private:
        static constexpr size_t mc_heap_table_column_count = 4u;
        static constexpr size_t mc_task_table_column_count = 3u;
        static constexpr size_t mc_queue_table_column_count = 5u;
        static constexpr size_t mc_freertos_queue_table_column_count = 6u;

        static const String_table_generator::Column_t mc_heap_table_columns[mc_heap_table_column_count];
        static const String_table_generator::Layout_t mc_heap_table_layout;
        static const String_table_generator::Column_t mc_task_table_columns[mc_task_table_column_count];
        static const String_table_generator::Layout_t mc_task_table_layout;
        static const String_table_generator::Column_t mc_queue_table_columns[mc_queue_table_column_count];
        static const String_table_generator::Layout_t mc_queue_table_layout;
        static const String_table_generator::Column_t mc_freertos_queue_table_columns[mc_freertos_queue_table_column_count];
        static const String_table_generator::Layout_t mc_freertos_queue_table_layout;

        void generate_heap_profile( Text_output& output );
        void generate_task_profile( Text_output& output );
        void generate_queue_profile( Text_output& output );
        void generate_freertos_queue_profile( Text_output& output );

        String_view byte_count_str( size_t size_B,
                                    String& out_string ) const;
        String_view percentage_str( size_t value,
                                    size_t capacity,
                                    String& out_string ) const;
        String_view stack_size_str( size_t size_B,
                                    String& out_string ) const;
        String_view stack_peak_str( size_t stack_depth_words,
                                    size_t stack_high_water_mark_words,
                                    String& out_string ) const;
    };
} // namespace SOMA

#endif // SOMA_MEMORY_PROFILER_H_
