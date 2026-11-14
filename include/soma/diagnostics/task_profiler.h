// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TASK_PROFILER_H_
#define SOMA_TASK_PROFILER_H_

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/containers/buffer.h"
#include "soma/assert.h"
#include "soma/text/string.h"
#include "soma/diagnostics/log.h"
#include "soma/runtime/mutex.h"
#include "soma/runtime/mutex_guard.h"
#include "soma/runtime/task.h"
#include "soma/text/string_table_generator.h"
#include "soma/text/string_view.h"
#include "soma/runtime/task_startup_event.h"
#include "soma/runtime/task_timer.h"
#include "soma/text/text_output.h"
#include "soma/time.h"

namespace SOMA
{
    class Event_task;

    namespace Internal
    {
        struct Task_profiler_cursor_t
        {
            uint32_t last_task_number = 0u;
            bool has_last_task_number = false;
        };

        uint32_t task_profiler_total_runtime( void );
        size_t task_profiler_active_task_count( void );
        bool task_profiler_next_task( Task_profiler_cursor_t& cursor,
                                      char* task_name,
                                      size_t task_name_capacity,
                                      uint32_t& task_number,
                                      int& task_state,
                                      uint32_t& base_priority,
                                      uint32_t& runtime_counter );
    }

    ///
    /// @brief Periodically collects active FreeRTOS task statistics.
    ///
    /// CPU usage windows and maximum retained task count are compile-time
    /// configuration.
    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    class Task_profiler
    {
    public:
        static_assert( Scan_period_ms > 0u, "The task-profiler scan period must be positive." );
        static_assert( First_CPU_usage_scan_period_multiplier > 0u,
                       "The first CPU-usage period multiplier must be positive." );
        static_assert( Second_CPU_usage_scan_period_multiplier > 0u,
                       "The second CPU-usage period multiplier must be positive." );
        static_assert( Max_task_count > 0u, "The task-profiler maximum task count must be positive." );
        static_assert( First_CPU_usage_scan_period_multiplier <= ( std::numeric_limits<uint32_t>::max() / Scan_period_ms ),
                       "The first CPU-usage period overflows milliseconds." );
        static_assert( Second_CPU_usage_scan_period_multiplier <= ( std::numeric_limits<uint32_t>::max() / Scan_period_ms ),
                       "The second CPU-usage period overflows milliseconds." );

        ///
        /// @brief Read-only statistics retained for one FreeRTOS task.
        class Task_profile
        {
        public:
            enum class State_e : uint8_t
            {
                running,
                ready,
                blocked,
                suspended,
                invalid
            };

            static constexpr size_t mc_CPU_usage_column_count = 3u;
            static constexpr size_t mc_name_max_length = configMAX_TASK_NAME_LEN - 1u;

            uint32_t number( void ) const;
            String_view name( void ) const;
            State_e state( void ) const;
            Task::Priority_e base_priority( void ) const;
            uint32_t CPU_utilization_percent_x100( size_t period_index ) const;

        private:
            friend class Task_profiler;

            char m_name[mc_name_max_length + 1u] {};
            uint32_t m_task_number = 0u;
            State_e m_state = State_e::invalid;
            Task::Priority_e m_base_priority { Task::Priority_e::idle };
            uint32_t m_CPU_utilization_percent_x100[mc_CPU_usage_column_count] {};
        };

        explicit Task_profiler( Event_task& event_task );

        ///
        /// @brief On-stack copy of retained task profiles and scan metadata.
        class Profile_snapshot
        {
        public:
            const Task_profile* begin( void ) const;
            const Task_profile* end( void ) const;
            size_t size( void ) const;
            size_t task_count( void ) const;
            uint32_t CPU_usage_period_ms( size_t period_index ) const;
            Time::Stamp timestamp( void ) const;

        private:
            friend class Task_profiler;

            explicit Profile_snapshot( Task_profiler& profiler );

            Task_profile m_profiles[Max_task_count];
            size_t m_size = 0u;
            size_t m_task_count = 0u;
            Time::Stamp m_timestamp { 0u };
        };

        Profile_snapshot profiles( void );
        void to_text( Text_output& output );

    private:
        static constexpr size_t mc_CPU_usage_column_count = Task_profile::mc_CPU_usage_column_count;
        static constexpr uint32_t mc_max_CPU_usage_scan_period_multiplier =
            ( First_CPU_usage_scan_period_multiplier > Second_CPU_usage_scan_period_multiplier )
            ? First_CPU_usage_scan_period_multiplier
            : Second_CPU_usage_scan_period_multiplier;

        struct Task_record_t
        {
            Task_profile m_profile;
            uint32_t m_runtime_counter_history[mc_max_CPU_usage_scan_period_multiplier] {};
            uint32_t m_scan_generation = 0u;
        };

        struct Scan_context_t
        {
            bool m_has_CPU_history_sample;
            uint32_t m_total_runtime_counter;
            uint32_t m_CPU_history_generation;
        };

        class Renderer
        {
        public:
            static void to_text( Task_profiler& profiler,
                                 Text_output& output );

        private:
            static constexpr size_t mc_CPU_usage_header_capacity =
                sizeof( "CPU [%] " ) - 1u + std::numeric_limits<uint32_t>::digits10 + 3u;

            struct CPU_usage_header_t
            {
                char m_text[mc_CPU_usage_header_capacity + 1u] {};
                uint8_t m_length = 0u;
            };

            static constexpr size_t mc_table_column_count = 2u + mc_CPU_usage_column_count + 2u;

            static const CPU_usage_header_t mc_CPU_usage_header_0;
            static const CPU_usage_header_t mc_CPU_usage_header_1;
            static const CPU_usage_header_t mc_CPU_usage_header_2;
            static const String_table_generator::Column_t mc_table_columns[mc_table_column_count];
            static const String_table_generator::Layout_t mc_table_layout;

            static constexpr void append( CPU_usage_header_t& header,
                                          char character );
            static constexpr void append( CPU_usage_header_t& header,
                                          uint32_t number );

            static constexpr CPU_usage_header_t make_CPU_usage_header( uint32_t period_ms );

            static String_view task_state_to_text( typename Task_profile::State_e task_state );
            static String_view task_priority_to_text( Task::Priority_e priority,
                                                      String& out_string );
            static String_view task_CPU_utilization_to_text( const Task_profile& profile,
                                                             size_t period_index,
                                                             String& out_string );
            static void task_record_to_text( const Task_profile& profile,
                                             String_table_generator& table );
            static void uptime_to_text( uint32_t uptime_ms_value,
                                        Text_output& output );
        };

        Task_record_t m_profile_storage[Max_task_count];
        Buffer<Task_record_t> m_profiles;
        Mutex m_profile_mutex;
        Task_timer<Task_profiler> m_scan_timer;
        Task_startup_event<Task_profiler> m_startup_event;

        Time::Stamp m_last_scan_timestamp { 0u };
        uint32_t m_total_runtime_counter_history[mc_max_CPU_usage_scan_period_multiplier] {};
        size_t m_task_count = 0u;
        uint32_t m_scan_generation = 0u;
        uint32_t m_CPU_history_generation = 0u;
        bool m_has_baseline = false;

        void collect_active_tasks( const Scan_context_t& context );
        void collect_task( uint32_t task_number,
                           String_view name,
                           int task_state,
                           uint32_t base_priority,
                           uint32_t runtime_counter,
                           const Scan_context_t& context );
        Task_record_t* find_profile( uint32_t task_number );
        Task_record_t* create_profile( uint32_t task_number,
                                       String_view name );
        void prune_profiles( uint32_t scan_generation );
        void update_CPU_utilization( Task_record_t& record,
                                     const Scan_context_t& context,
                                     uint32_t task_runtime_counter );
        static constexpr uint32_t CPU_usage_scan_period_multiplier( size_t period_index );
        static constexpr uint32_t CPU_usage_period_ms( size_t period_index );
        static constexpr size_t CPU_history_slot( uint32_t history_generation );
        static typename Task_profile::State_e to_profile_state( int task_state );
        static uint32_t to_percent_x100( uint32_t task_runtime_counter,
                                         uint32_t total_runtime_counter );

        void scan( void );

        void on_startup( void );
        void on_scan_timer( void );
    };

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    uint32_t Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_profile::number( void ) const
    {
        return m_task_number;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    String_view Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                              Second_CPU_usage_scan_period_multiplier>::Task_profile::name( void ) const
    {
        return String_view( m_name );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_profile::State_e
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Task_profile::state( void ) const
    {
        return m_state;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    Task::Priority_e Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                   Second_CPU_usage_scan_period_multiplier>::Task_profile::base_priority( void ) const
    {
        return m_base_priority;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    uint32_t Task_profiler<
        Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
        Second_CPU_usage_scan_period_multiplier>::Task_profile::CPU_utilization_percent_x100( size_t period_index ) const
    {
        ASSERT( period_index < mc_CPU_usage_column_count );

        return m_CPU_utilization_percent_x100[period_index];
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    constexpr void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                 Second_CPU_usage_scan_period_multiplier>::Renderer::append( CPU_usage_header_t& header,
                                                                                             char character )
    {
        header.m_text[header.m_length] = character;
        ++header.m_length;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    constexpr void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                 Second_CPU_usage_scan_period_multiplier>::Renderer::append( CPU_usage_header_t& header,
                                                                                             uint32_t number )
    {
        char reversed_digits[std::numeric_limits<uint32_t>::digits10 + 1u] {};
        size_t digit_count = 0u;

        do
        {
            reversed_digits[digit_count] = static_cast<char>( '0' + ( number % 10u ) );
            number /= 10u;
            ++digit_count;
        } while( number > 0u );

        while( digit_count > 0u )
        {
            --digit_count;
            append( header, reversed_digits[digit_count] );
        }
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    constexpr typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                     Second_CPU_usage_scan_period_multiplier>::Renderer::CPU_usage_header_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::make_CPU_usage_header( uint32_t period_ms )
    {
        constexpr char c_header_prefix[] = "CPU [%] ";

        CPU_usage_header_t header;
        const uint32_t fractional_ms = period_ms % 1000u;

        for( size_t prefix_index = 0u; prefix_index < ( sizeof( c_header_prefix ) - 1u ); ++prefix_index )
        {
            append( header, c_header_prefix[prefix_index] );
        }

        append( header, period_ms / 1000u );

        if( fractional_ms > 0u )
        {
            const uint32_t hundreds = fractional_ms / 100u;
            const uint32_t tens = ( fractional_ms / 10u ) % 10u;
            const uint32_t ones = fractional_ms % 10u;

            append( header, '.' );
            append( header, static_cast<char>( '0' + hundreds ) );

            if( ( tens > 0u ) || ( ones > 0u ) )
            {
                append( header, static_cast<char>( '0' + tens ) );
            }

            if( ones > 0u )
            {
                append( header, static_cast<char>( '0' + ones ) );
            }
        }

        append( header, 's' );

        return header;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    inline const typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                        Second_CPU_usage_scan_period_multiplier>::Renderer::CPU_usage_header_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::mc_CPU_usage_header_0 =
        make_CPU_usage_header( Scan_period_ms );

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    inline const typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                        Second_CPU_usage_scan_period_multiplier>::Renderer::CPU_usage_header_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::mc_CPU_usage_header_1 =
        make_CPU_usage_header( Scan_period_ms * First_CPU_usage_scan_period_multiplier );

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    inline const typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                        Second_CPU_usage_scan_period_multiplier>::Renderer::CPU_usage_header_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::mc_CPU_usage_header_2 =
        make_CPU_usage_header( Scan_period_ms * Second_CPU_usage_scan_period_multiplier );

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    inline const String_table_generator::Column_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::mc_table_columns[mc_table_column_count] = {
        { "Task #", 4u, String_table_generator::Cell_alignment_e::right },
        { "Task Name", static_cast<uint8_t>( Task_profile::mc_name_max_length ),
          String_table_generator::Cell_alignment_e::left },
        { String_view( mc_CPU_usage_header_0.m_text, make_CPU_usage_header( Scan_period_ms ).m_length ),
          8,
          String_table_generator::Cell_alignment_e::right },
        { String_view( mc_CPU_usage_header_1.m_text,
                       make_CPU_usage_header( Scan_period_ms * First_CPU_usage_scan_period_multiplier ).m_length ),
          8,
          String_table_generator::Cell_alignment_e::right },
        { String_view( mc_CPU_usage_header_2.m_text,
                       make_CPU_usage_header( Scan_period_ms * Second_CPU_usage_scan_period_multiplier ).m_length ),
          8,
          String_table_generator::Cell_alignment_e::right },
        { "State", 12u, String_table_generator::Cell_alignment_e::center },
        { "Priority", 12u, String_table_generator::Cell_alignment_e::center } };

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    inline const String_table_generator::Layout_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::mc_table_layout = { " ", '*',
                                                                                          Array( mc_table_columns ) };

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Task_profiler( Event_task& event_task )
        : m_profiles( Array( m_profile_storage ) ),
          m_scan_timer( event_task, *this, &Task_profiler::on_scan_timer ),
          m_startup_event( event_task, *this, &Task_profiler::on_startup )
    {
        // intentionally empty
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_profile const*
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::begin( void ) const
    {
        return m_profiles;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_profile const*
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::end( void ) const
    {
        return m_profiles + m_size;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    size_t Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                         Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::size( void ) const
    {
        return m_size;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    size_t Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                         Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::task_count( void ) const
    {
        return m_task_count;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    uint32_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::CPU_usage_period_ms( size_t period_index )
    const
    {
        return Task_profiler::CPU_usage_period_ms( period_index );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    Time::Stamp Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                              Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::timestamp( void ) const
    {
        return m_timestamp;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Profile_snapshot::Profile_snapshot( Task_profiler& profiler )
    {
        Mutex_guard mutex_guard( profiler.m_profile_mutex );

        const Array<const Task_record_t> profiles = profiler.m_profiles;

        for( const Task_record_t& record: profiles )
        {
            m_profiles[m_size] = record.m_profile;
            ++m_size;
        }

        m_task_count = profiler.m_task_count;
        m_timestamp = profiler.m_last_scan_timestamp;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Profile_snapshot
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::profiles( void )
    {
        return Profile_snapshot( *this );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::to_text( Text_output& output )
    {
        Renderer::to_text( *this, output );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::Renderer::to_text( Task_profiler& profiler,
                                                                                    Text_output& output )
    {
        const auto task_profiles = profiler.profiles();

        String_table_generator table { mc_table_layout, output };

        table.header();

        for( const Task_profile& profile: task_profiles )
        {
            task_record_to_text( profile, table );
        }

        output << EOL;

        if( task_profiles.task_count() > task_profiles.size() )
        {
            output << Color::yellow
                   << "WARNING: Only " << task_profiles.size() << " of " << task_profiles.task_count()
                   << " active tasks are shown!"
                   << Color::standard << EOL;
        }

        output << "Uptime: ";
        uptime_to_text( Time::uptime_ms(), output );
        output << EOL;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::Renderer::uptime_to_text( uint32_t uptime_ms_value,
                                                                                           Text_output& output )
    {
        constexpr uint32_t c_milliseconds_per_second = 1000u;
        constexpr uint32_t c_seconds_per_minute = 60u;
        constexpr uint32_t c_minutes_per_hour = 60u;
        constexpr uint32_t c_hours_per_day = 24u;

        uint32_t remaining_seconds = uptime_ms_value / c_milliseconds_per_second;
        const uint32_t milliseconds = uptime_ms_value % c_milliseconds_per_second;
        const uint32_t seconds = remaining_seconds % c_seconds_per_minute;

        remaining_seconds /= c_seconds_per_minute;

        const uint32_t minutes = remaining_seconds % c_minutes_per_hour;

        remaining_seconds /= c_minutes_per_hour;

        const uint32_t hours = remaining_seconds % c_hours_per_day;
        const uint32_t days = remaining_seconds / c_hours_per_day;

        output << days << "d "
               << min_width( 2u ) << fill_with( '0' ) << hours << ":"
               << min_width( 2u ) << fill_with( '0' ) << minutes << ":"
               << min_width( 2u ) << fill_with( '0' ) << seconds << "."
               << min_width( 3u ) << fill_with( '0' ) << milliseconds;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    String_view
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::task_state_to_text( typename Task_profile::State_e
                                                                                          task_state )
    {
        String_view task_state_text = "invalid";

        switch( task_state )
        {
            case Task_profile::State_e::running:
                task_state_text = "running";
                break;

            case Task_profile::State_e::ready:
                task_state_text = "ready";
                break;

            case Task_profile::State_e::blocked:
                task_state_text = "blocked";
                break;

            case Task_profile::State_e::suspended:
                task_state_text = "suspended";
                break;

            case Task_profile::State_e::invalid:
                [[fallthrough]];

            default:
                break;
        }

        return task_state_text;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    String_view
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::task_priority_to_text( Task::Priority_e priority,
                                                                                             [[maybe_unused]] String&
                                                                                             out_string )
    {
        String_view priority_text = "invalid";

#if configMAX_PRIORITIES == 7
        {
            switch( priority )
            {
                case Task::Priority_e::idle:
                    priority_text = "idle";
                    break;

                case Task::Priority_e::low:
                    priority_text = "low";
                    break;

                case Task::Priority_e::below_normal:
                    priority_text = "below normal";
                    break;

                case Task::Priority_e::normal:
                    priority_text = "normal";
                    break;

                case Task::Priority_e::above_normal:
                    priority_text = "above normal";
                    break;

                case Task::Priority_e::high:
                    priority_text = "high";
                    break;

                case Task::Priority_e::realtime:
                    priority_text = "realtime";
                    break;

                default:
                    break;
            }
        }
#else
        {
            out_string.clear();
            out_string += static_cast<UBaseType_t>( priority );

            priority_text = out_string.view();
        }
#endif

        return priority_text;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    String_view
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::Renderer::task_CPU_utilization_to_text( const Task_profile&
                                                                                                    profile,
                                                                                                    size_t period_index,
                                                                                                    String& out_string )
    {
        constexpr size_t c_decimal_digit_count = 2u;

        out_string.clear();
        out_string.fraction( profile.CPU_utilization_percent_x100( period_index ),
                             100u,
                             "%" PRIu32,
                             c_decimal_digit_count );

        return out_string.view();
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::Renderer::task_record_to_text( const Task_profile& profile,
                                                                                                String_table_generator&
                                                                                                table )
    {
        String::Length<20u> string;

        table << profile.number() << profile.name();

        for( size_t period_index = 0u; period_index < Task_profile::mc_CPU_usage_column_count; ++period_index )
        {
            table << task_CPU_utilization_to_text( profile, period_index, string );
        }

        table << task_state_to_text( profile.state() ) << task_priority_to_text( profile.base_priority(), string );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::on_startup( void )
    {
        // FreeRTOS configures the run-time timer when the scheduler starts.
        // Restarting it here would invalidate the already accumulated per-task
        // counters.
        scan();
        m_scan_timer.start_periodic( Scan_period_ms );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::on_scan_timer( void )
    {
        scan();
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::scan( void )
    {
        Mutex_guard mutex_guard( m_profile_mutex );

        const Time::Stamp timestamp = Time::Stamp::now();
        const uint32_t total_runtime_counter = Internal::task_profiler_total_runtime();
        const size_t task_count = Internal::task_profiler_active_task_count();
        const intmax_t elapsed_ms = timestamp - m_last_scan_timestamp;
        const bool has_elapsed_interval = m_has_baseline && ( elapsed_ms > 0 );
        const bool has_CPU_history_sample = ( m_has_baseline == false ) || has_elapsed_interval;

        ++m_scan_generation;

        if( m_scan_generation == 0u )
        {
            ++m_scan_generation;
        }

        if( has_CPU_history_sample )
        {
            ++m_CPU_history_generation;

            if( m_CPU_history_generation == 0u )
            {
                ++m_CPU_history_generation;
            }
        }

        const Scan_context_t context { has_CPU_history_sample, total_runtime_counter, m_CPU_history_generation };

        collect_active_tasks( context );

        if( has_CPU_history_sample )
        {
            m_total_runtime_counter_history[CPU_history_slot( m_CPU_history_generation )] = total_runtime_counter;
        }

        prune_profiles( m_scan_generation );

        m_task_count = task_count;
        m_last_scan_timestamp = timestamp;
        m_has_baseline = true;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::collect_active_tasks( const Scan_context_t& context )
    {
        char task_name[Task_profile::mc_name_max_length + 1u] {};
        Internal::Task_profiler_cursor_t cursor;
        uint32_t task_number = 0u;
        int task_state = 0;
        uint32_t base_priority = 0u;
        uint32_t runtime_counter = 0u;

        while( Internal::task_profiler_next_task( cursor,
                                                  task_name,
                                                  sizeof( task_name ),
                                                  task_number,
                                                  task_state,
                                                  base_priority,
                                                  runtime_counter ) )
        {
            collect_task( task_number,
                          String_view( task_name ),
                          task_state,
                          base_priority,
                          runtime_counter,
                          context );
        }
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms,
                       First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::collect_task( uint32_t task_number,
                                                                               String_view name,
                                                                               int task_state,
                                                                               uint32_t base_priority,
                                                                               uint32_t runtime_counter,
                                                                               const Scan_context_t& context )
    {
        Task_record_t* p_record = find_profile( task_number );

        if( p_record == nullptr )
        {
            p_record = create_profile( task_number, name );
        }

        if( p_record != nullptr )
        {
            Task_profile& profile = p_record->m_profile;

            profile.m_state = to_profile_state( task_state );
            profile.m_base_priority = Task::Priority_e( base_priority );
            p_record->m_scan_generation = m_scan_generation;

            update_CPU_utilization( *p_record, context, runtime_counter );

            if( context.m_has_CPU_history_sample )
            {
                p_record->m_runtime_counter_history[CPU_history_slot( context.m_CPU_history_generation )] = runtime_counter;
            }
        }
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_record_t*
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::find_profile( uint32_t task_number )
    {
        Array<Task_record_t> profiles = m_profiles;
        Task_record_t* p_profile = nullptr;

        for( Task_record_t& record: profiles )
        {
            if( record.m_profile.m_task_number == task_number )
            {
                p_profile = &record;
                break;
            }
        }

        return p_profile;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_record_t*
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::create_profile( uint32_t task_number,
                                                                            String_view name )
    {
        Task_record_t record;
        Task_profile& profile = record.m_profile;
        Task_record_t* p_created_profile = nullptr;

        profile.m_task_number = task_number;

        size_t name_length = name.length();

        if( name_length > Task_profile::mc_name_max_length )
        {
            name_length = Task_profile::mc_name_max_length;
        }

        for( size_t i = 0u; i < name_length; ++i )
        {
            profile.m_name[i] = name[i];
        }

        profile.m_name[name_length] = '\0';

        if( m_profiles.insert( record ) )
        {
            Array<Task_record_t> profiles = m_profiles;

            p_created_profile = &( profiles[profiles.size() - 1u] );
        }

        return p_created_profile;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::prune_profiles( uint32_t scan_generation )
    {
        Array<Task_record_t> profiles = m_profiles;
        size_t retained_profile_count = 0u;

        for( size_t i = 0u; i < profiles.size(); ++i )
        {
            if( profiles[i].m_scan_generation == scan_generation )
            {
                if( retained_profile_count != i )
                {
                    profiles[retained_profile_count] = profiles[i];
                }

                ++retained_profile_count;
            }
        }

        m_profiles.displace( m_profiles.size() - retained_profile_count );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    void Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                       Second_CPU_usage_scan_period_multiplier>::update_CPU_utilization( Task_record_t& record,
                                                                                         const Scan_context_t& context,
                                                                                         uint32_t task_runtime_counter )
    {
        if( context.m_has_CPU_history_sample )
        {
            for( size_t period_index = 0u; period_index < mc_CPU_usage_column_count; ++period_index )
            {
                const uint32_t multiplier = CPU_usage_scan_period_multiplier( period_index );
                const uint32_t start_generation =
                    ( context.m_CPU_history_generation >= multiplier ) ? context.m_CPU_history_generation - multiplier : 0u;
                const uint32_t task_runtime_counter_start =
                    record.m_runtime_counter_history[CPU_history_slot( start_generation )];
                const uint32_t total_runtime_counter_start =
                    m_total_runtime_counter_history[CPU_history_slot( start_generation )];
                const uint32_t task_runtime_delta_window =
                    task_runtime_counter - task_runtime_counter_start;
                const uint32_t total_runtime_delta_window =
                    context.m_total_runtime_counter - total_runtime_counter_start;

                record.m_profile.m_CPU_utilization_percent_x100[period_index] =
                    to_percent_x100( task_runtime_delta_window, total_runtime_delta_window );
            }
        }
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    constexpr uint32_t
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::CPU_usage_scan_period_multiplier( size_t period_index )
    {
        uint32_t multiplier = 1u;

        switch( period_index )
        {
            case 0u:
                break;

            case 1u:
                multiplier = First_CPU_usage_scan_period_multiplier;
                break;

            case 2u:
                multiplier = Second_CPU_usage_scan_period_multiplier;
                break;

            default:
                ASSERT( false );
                break;
        }

        return multiplier;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    constexpr uint32_t Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                     Second_CPU_usage_scan_period_multiplier>::CPU_usage_period_ms( size_t period_index )
    {
        return Scan_period_ms * CPU_usage_scan_period_multiplier( period_index );
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    constexpr size_t Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                                   Second_CPU_usage_scan_period_multiplier>::CPU_history_slot( uint32_t history_generation )
    {
        return history_generation % mc_max_CPU_usage_scan_period_multiplier;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    typename Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::Task_profile::State_e
    Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                  Second_CPU_usage_scan_period_multiplier>::to_profile_state( int task_state )
    {
        typename Task_profile::State_e profile_state = Task_profile::State_e::invalid;

        switch( static_cast<eTaskState>( task_state ) )
        {
            case eRunning:
                profile_state = Task_profile::State_e::running;
                break;

            case eReady:
                profile_state = Task_profile::State_e::ready;
                break;

            case eBlocked:
                profile_state = Task_profile::State_e::blocked;
                break;

            case eSuspended:
                profile_state = Task_profile::State_e::suspended;
                break;

            case eDeleted:
                [[fallthrough]];

            case eInvalid:
                [[fallthrough]];

            default:
                break;
        }

        return profile_state;
    }

    template<size_t Max_task_count,
             uint32_t Scan_period_ms,
             uint32_t First_CPU_usage_scan_period_multiplier,
             uint32_t Second_CPU_usage_scan_period_multiplier>
    uint32_t Task_profiler<Max_task_count, Scan_period_ms, First_CPU_usage_scan_period_multiplier,
                           Second_CPU_usage_scan_period_multiplier>::to_percent_x100( uint32_t task_runtime_counter,
                                                                                      uint32_t total_runtime_counter )
    {
        uint32_t percent_x100;

        if( total_runtime_counter == 0u )
        {
            percent_x100 = 0u;
        }
        else if( task_runtime_counter >= total_runtime_counter )
        {
            percent_x100 = 10000u;
        }
        else
        {
            const uint64_t scaled_task_runtime_counter =
                static_cast<uint64_t>( task_runtime_counter ) * 10000u;

            percent_x100 = static_cast<uint32_t>( scaled_task_runtime_counter /
                                                  static_cast<uint64_t>( total_runtime_counter ) );
        }

        return percent_x100;
    }
} // namespace SOMA

#endif // TASK_PROFILER_H_
