// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "gtest/gtest.h"

#include "soma/io/blocking_output_stream.h"
#include "soma/runtime/event_task.h"
#include "soma/text/string.h"
#include "soma/io/i_output_stream.h"
#include "soma/runtime/task_memory.h"
#include "soma/diagnostics/task_profiler.h"

using namespace SOMA;

namespace
{

constexpr uint32_t c_scan_period_ms = 100u;
constexpr uint32_t c_first_CPU_usage_scan_period_multiplier = 2u;
constexpr uint32_t c_second_CPU_usage_scan_period_multiplier = 3u;
volatile bool g_CPU_load_task_is_busy = false;

template<size_t N>
using Test_task_profiler = Task_profiler<N, c_scan_period_ms, c_first_CPU_usage_scan_period_multiplier,
                                           c_second_CPU_usage_scan_period_multiplier>;

template<size_t N> using Test_task_profile = typename Test_task_profiler<N>::Task_profile;

bool contains( String_view text, String_view expected )
{
    size_t position = 0u;

    return text.find( expected, position );
}

class Test_output_stream: public I_output_stream
{
public:
    explicit Test_output_stream( String& output_text )
        : I_output_stream( "task_profiler_test_output" ), m_output_text( output_text )
    {
        // intentionally empty
    }

    Result_e write( Buffer_view<uint8_t> data ) override
    {
        m_output_text += String_view( data );

        return Result_e::ready;
    }

private:
    String& m_output_text;
};

template<size_t N> class Active_task_profiler
{
public:
    Active_task_profiler( void ) : m_event_task( "profiler", Task::Priority_e::low, m_task_memory ), m_profiler( m_event_task )
    {
        // intentionally empty
    }

    ~Active_task_profiler( void )
    {
        m_event_task.stop();
        Task::sleep( 1u );
    }

    void start( void )
    {
        m_event_task.start();

        // Let the lower-priority profiler task take its startup baseline
        // before this higher-priority test task starts taking read leases.
        Task::sleep( 1u );

        while( true )
        {
            const auto profiles = m_profiler.profiles();

            if( profiles.size() > 0u )
            {
                break;
            }

            Task::sleep( 1u );
        }
    }

    void wait_for_next_scan( void )
    {
        const Time::Stamp previous_timestamp = scan_timestamp();

        do
        {
            Task::sleep( 1u );
        } while( scan_timestamp() == previous_timestamp );
    }

    Time::Stamp scan_timestamp( void )
    {
        const auto profiles = m_profiler.profiles();

        return profiles.timestamp();
    }

    Test_task_profiler<N>& profiler( void )
    {
        return m_profiler;
    }

private:
    Task_memory<800u> m_task_memory;
    Event_task m_event_task;
    Test_task_profiler<N> m_profiler;
};

template<size_t N> void render_profiler( Active_task_profiler<N>& active_profiler, String& output_text )
{
    Test_output_stream output_stream { output_text };
    Blocking_output_stream blocking_output_stream { output_stream };
    Text_output output { blocking_output_stream };

    active_profiler.profiler().to_text( output );
}

template<typename Profile_snapshot>
auto find_profile_named( const Profile_snapshot& profiles, String_view name ) -> decltype( &( *profiles.begin() ) )
{
    for( const auto& profile : profiles )
    {
        if( profile.name() == name )
        {
            return &profile;
        }
    }

    return nullptr;
}

void dormant_task( [[maybe_unused]] void* parameter )
{
    Task::sleep( 1000u );
}

void notification_wait_task( [[maybe_unused]] void* parameter )
{
    (void)ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    vTaskDelete( nullptr );
}

void suspended_task( [[maybe_unused]] void* parameter )
{
    vTaskSuspend( nullptr );
}

bool wait_for_task_state( TaskHandle_t task_handle, eTaskState expected_state )
{
    constexpr uint32_t c_timeout_ms = 100u;

    for( uint32_t elapsed_ms = 0u; elapsed_ms < c_timeout_ms; ++elapsed_ms )
    {
        if( eTaskGetState( task_handle ) == expected_state )
        {
            return true;
        }

        Task::sleep( 1u );
    }

    return false;
}

void CPU_load_task( [[maybe_unused]] void* parameter )
{
    while( true )
    {
        if( g_CPU_load_task_is_busy )
        {
            taskYIELD();
        }
        else
        {
            Task::sleep( 1u );
        }
    }
}

} // namespace

TEST( task_profiler, snapshot_exposes_profiles_as_const_references )
{
    using Task_profile = Test_task_profile<8u>;

    Active_task_profiler<8u> active_profiler;

    active_profiler.start();

    const auto profiles = active_profiler.profiler().profiles();
    size_t iterated_profile_count = 0u;
    uint32_t previous_task_number = 0u;
    bool has_previous_task_number = false;

    for( const Task_profile& profile : profiles )
    {
        if( has_previous_task_number )
        {
            EXPECT_GT( profile.number(), previous_task_number );
        }

        EXPECT_FALSE( profile.name().is_empty() );
        EXPECT_NE( profile.state(), Task_profile::State_e::invalid );

        for( size_t period_index = 0u; period_index < Task_profile::mc_CPU_usage_column_count; ++period_index )
        {
            EXPECT_LE( profile.CPU_utilization_percent_x100( period_index ), 10000u );
        }

        [[maybe_unused]] const Task::Priority_e priority = profile.base_priority();

        previous_task_number = profile.number();
        has_previous_task_number = true;
        ++iterated_profile_count;
    }

    EXPECT_EQ( iterated_profile_count, profiles.size() );
    EXPECT_EQ( profiles.CPU_usage_period_ms( 0u ), c_scan_period_ms );
    EXPECT_EQ( profiles.CPU_usage_period_ms( 1u ), c_scan_period_ms * c_first_CPU_usage_scan_period_multiplier );
    EXPECT_EQ( profiles.CPU_usage_period_ms( 2u ), c_scan_period_ms * c_second_CPU_usage_scan_period_multiplier );
}

TEST( task_profiler, distinguishes_indefinite_notification_wait_from_suspension )
{
    using Task_profile = Test_task_profile<8u>;

    TaskHandle_t notification_wait_handle = nullptr;
    TaskHandle_t suspended_handle = nullptr;

    ASSERT_EQ( xTaskCreate( notification_wait_task, "notify_wait", configMINIMAL_STACK_SIZE, nullptr, 1u,
                            &notification_wait_handle ),
               pdPASS );
    ASSERT_EQ( xTaskCreate( suspended_task, "suspended", configMINIMAL_STACK_SIZE, nullptr, 1u, &suspended_handle ),
               pdPASS );

    ASSERT_TRUE( wait_for_task_state( notification_wait_handle, eBlocked ) );
    ASSERT_TRUE( wait_for_task_state( suspended_handle, eSuspended ) );

    Active_task_profiler<8u> active_profiler;

    active_profiler.start();

    {
        const auto profiles = active_profiler.profiler().profiles();
        const auto* p_notification_wait_profile = find_profile_named( profiles, "notify_wait" );
        const auto* p_suspended_profile = find_profile_named( profiles, "suspended" );

        ASSERT_NE( p_notification_wait_profile, nullptr );
        ASSERT_NE( p_suspended_profile, nullptr );
        EXPECT_EQ( p_notification_wait_profile->state(), Task_profile::State_e::blocked );
        EXPECT_EQ( p_suspended_profile->state(), Task_profile::State_e::suspended );
    }

    vTaskDelete( notification_wait_handle );
    vTaskDelete( suspended_handle );
    active_profiler.wait_for_next_scan();
}

TEST( task_profiler, retained_snapshot_survives_task_deletion_while_new_snapshot_prunes_task )
{
    TaskHandle_t task_handle = nullptr;

    ASSERT_EQ( xTaskCreate( dormant_task, "removed_task", configMINIMAL_STACK_SIZE, nullptr, 1u, &task_handle ),
               pdPASS );

    Active_task_profiler<8u> active_profiler;

    active_profiler.start();

    const auto retained_snapshot = active_profiler.profiler().profiles();
    const auto* p_removed_profile = find_profile_named( retained_snapshot, "removed_task" );

    ASSERT_NE( p_removed_profile, nullptr );

    vTaskDelete( task_handle );
    active_profiler.wait_for_next_scan();

    EXPECT_EQ( p_removed_profile->name(), "removed_task" );

    const auto current_snapshot = active_profiler.profiler().profiles();

    EXPECT_EQ( find_profile_named( current_snapshot, "removed_task" ), nullptr );
}

TEST( task_profiler, refreshes_rolling_CPU_utilization_on_every_base_scan )
{
    Active_task_profiler<8u> active_profiler;
    TaskHandle_t task_handle = nullptr;

    active_profiler.start();

    ASSERT_EQ( xTaskCreate( CPU_load_task, "CPU_load", configMINIMAL_STACK_SIZE, nullptr, 1u, &task_handle ), pdPASS );
    g_CPU_load_task_is_busy = true;

    active_profiler.wait_for_next_scan();
    active_profiler.wait_for_next_scan();
    active_profiler.wait_for_next_scan();

    uint32_t busy_CPU_utilization = 0u;

    {
        const auto profiles = active_profiler.profiler().profiles();
        const auto* p_load_task = find_profile_named( profiles, "CPU_load" );

        ASSERT_NE( p_load_task, nullptr );
        busy_CPU_utilization = p_load_task->CPU_utilization_percent_x100( 1u );
    }

    g_CPU_load_task_is_busy = false;
    active_profiler.wait_for_next_scan();

    {
        const auto profiles = active_profiler.profiler().profiles();
        const auto* p_load_task = find_profile_named( profiles, "CPU_load" );

        ASSERT_NE( p_load_task, nullptr );
        EXPECT_LT( p_load_task->CPU_utilization_percent_x100( 1u ), busy_CPU_utilization );
    }

    vTaskDelete( task_handle );
    active_profiler.wait_for_next_scan();
}

TEST( task_profiler, renders_CPU_utilization_for_new_tasks_from_zero_initialized_history )
{
    Active_task_profiler<8u> active_profiler;
    TaskHandle_t task_handle = nullptr;
    String::Length<2048u> rendered_text;

    active_profiler.start();

    ASSERT_EQ( xTaskCreate( dormant_task, "new_task", configMINIMAL_STACK_SIZE, nullptr, 1u, &task_handle ), pdPASS );

    active_profiler.wait_for_next_scan();

    {
        const auto profiles = active_profiler.profiler().profiles();
        const auto* p_new_task = find_profile_named( profiles, "new_task" );

        ASSERT_NE( p_new_task, nullptr );
    }

    render_profiler( active_profiler, rendered_text );
    EXPECT_FALSE( contains( rendered_text, "--.--" ) );

    vTaskDelete( task_handle );
    active_profiler.wait_for_next_scan();
}

TEST( task_profiler, renders_hidden_tasks_without_stack_data )
{
    Active_task_profiler<1u> active_profiler;
    String::Length<2048u> rendered_text;
    String::Length<Test_task_profile<1u>::mc_name_max_length> profile_name;

    active_profiler.start();

    {
        const auto profiles = active_profiler.profiler().profiles();

        ASSERT_GT( profiles.task_count(), profiles.size() );
        ASSERT_EQ( profiles.size(), 1u );

        profile_name = ( *profiles.begin() ).name();
    }

    render_profiler( active_profiler, rendered_text );

    EXPECT_TRUE( contains( rendered_text, profile_name ) );
    EXPECT_FALSE( contains( rendered_text, "--.--" ) );
}
