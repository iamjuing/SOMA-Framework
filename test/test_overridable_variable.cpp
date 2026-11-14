// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"
#include "mock_variable_observer.h"

#include "soma/variables/overridable_variable.h"
#include "soma/variables/variable_observer_handler.h"
#include "soma/text/text_string_input.h"

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

using namespace SOMA;

namespace
{

    class Override_status_observer
    {
    public:
        explicit Override_status_observer( Overridable_variable<int>& variable )
            : m_variable( variable ),
              m_handler( variable, *this, &Override_status_observer::on_change )
        {
        }

        int values[2] {};
        bool statuses[2] {};
        size_t notifications = 0;

    private:
        void on_change()
        {
            if( notifications < 2 )
            {
                values[notifications] = m_variable.get();
                statuses[notifications] = m_variable.is_overridden();
            }
            ++notifications;
        }

        Overridable_variable<int>& m_variable;
        Variable_observer_handler<Override_status_observer> m_handler;
    };

    struct Concurrent_clear_context
    {
        Overridable_variable<int>& variable;
        std::atomic_bool start { false };
        std::atomic_bool attempted { false };
        std::atomic_bool completed { false };
    };

    void concurrent_clear_task( void* context )
    {
        auto& state = *static_cast<Concurrent_clear_context*>( context );
        while( state.start.load() == false )
        {
            vTaskDelay( 1 );
        }

        state.attempted.store( true );
        state.variable.clear_override();
        state.completed.store( true );
        vTaskSuspend( nullptr );
    }

    class Blocking_override_observer
    {
    public:
        Blocking_override_observer( Overridable_variable<int>& variable,
                                    Concurrent_clear_context& state )
            : m_state( state ),
              m_handler( variable, *this, &Blocking_override_observer::on_change )
        {
        }

        bool clear_attempted_during_callback = false;
        bool clear_completed_during_callback = false;

    private:
        void on_change()
        {
            if( m_first_notification.exchange( false ) )
            {
                m_state.start.store( true );
                clear_attempted_during_callback = wait_until_true( 200u, [&]() { return m_state.attempted.load(); } );
                vTaskDelay( 2 );
                clear_completed_during_callback = m_state.completed.load();
            }
        }

        Concurrent_clear_context& m_state;
        Variable_observer_handler<Blocking_override_observer> m_handler;
        std::atomic_bool m_first_notification { true };
    };

    struct Owner_publication_context
    {
        Overridable_variable<int>& variable;
        std::atomic_bool finished { false };
    };

    void owner_publication_task( void* context )
    {
        auto& state = *static_cast<Owner_publication_context*>( context );
        state.variable = 20;
        state.finished.store( true );
        vTaskSuspend( nullptr );
    }

    class Publishing_task_observer
    {
    public:
        explicit Publishing_task_observer( Overridable_variable<int>& variable )
            : m_variable( variable ),
              m_handler( variable, *this, &Publishing_task_observer::on_change )
        {
        }

        std::atomic<TaskHandle_t> callback_task { nullptr };
        std::atomic<int> observed_value { 0 };
        std::atomic<unsigned> notifications { 0 };

    private:
        void on_change()
        {
            observed_value.store( m_variable.get() );
            callback_task.store( xTaskGetCurrentTaskHandle() );
            notifications.fetch_add( 1 );
        }

        Overridable_variable<int>& m_variable;
        Variable_observer_handler<Publishing_task_observer> m_handler;
    };
}

struct Overridable_variable_test_type_t
{
    int32_t a;
    int32_t b;

    bool operator==( const Overridable_variable_test_type_t& rhs ) const
    {
        bool rv_equal = true;

        rv_equal = rv_equal && ( a == rhs.a );
        rv_equal = rv_equal && ( b == rhs.b );

        return rv_equal;
    }
};

Text_output& operator<<( Text_output& str,
                         const Overridable_variable_test_type_t value )
{
    // intentionally empty

    return str;
}

bool from_text( String_view value_string,
                Overridable_variable_test_type_t& out_value )
{
    // expected format: "a,b" where a and b are integers
    Text_string_input string_input( value_string );
    int32_t a, b;
    char comma;

    if( ( string_input >> a >> comma >> b ) && comma == ',' )
    {
        out_value.a = a;
        out_value.b = b;

        return true;
    }

    return false;
}

TEST( overridable_variable, explicit_initial_value_starts_unoverridden )
{
    Overridable_variable<int> P_tested( 0 );

    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_TRUE( P_tested.is_overridable() );
}

TEST( overridable_variable, value_constructor )
{
    Overridable_variable<int> P_tested( 42 );

    ASSERT_EQ( P_tested, 42 );
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_TRUE( P_tested.is_overridable() );
}

TEST( overridable_variable, set_value )
{
    Overridable_variable<int> P_tested( 0 );
    Mock_variable_observer<int> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 10 ) );
    EXPECT_CALL( observer, changed( 20 ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    P_tested = 10;
    ASSERT_EQ( P_tested, 10 );
    ASSERT_FALSE( P_tested.is_overridden() );

    P_tested = 20;
    ASSERT_EQ( P_tested, 20 );
    ASSERT_FALSE( P_tested.is_overridden() );
}

TEST( overridable_variable, set_struct_value )
{
    Overridable_variable_test_type_t current_value;
    Overridable_variable<Overridable_variable_test_type_t> P_tested( Overridable_variable_test_type_t {} );
    Mock_variable_observer<Overridable_variable_test_type_t> observer( P_tested );

    Overridable_variable_test_type_t init_value = { 4, 2 };
    Overridable_variable_test_type_t new_value = { 66, -666 };

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( _ ) ).Times( 0 );
    EXPECT_CALL( observer, changed( new_value ) ).Times( 1 );

    // **** TEST SEQUENCE ********************************************************************************************************

    P_tested.init( init_value );
    current_value = P_tested;
    ASSERT_MEM_EQ( &current_value, &init_value, sizeof( Overridable_variable_test_type_t ) );
    ASSERT_FALSE( P_tested.is_overridden() );

    P_tested = new_value;
    current_value = P_tested;
    ASSERT_MEM_EQ( &current_value, &new_value, sizeof( Overridable_variable_test_type_t ) );
    ASSERT_FALSE( P_tested.is_overridden() );
}

TEST( overridable_variable, simple_override )
{
    Overridable_variable<int> P_tested( 10 );
    Mock_variable_observer<int> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 50 ) );
    EXPECT_CALL( observer, changed( 10 ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 10 );

    // Override the variable
    ASSERT_TRUE( P_tested.set_override( 50 ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 50 );

    // Clear override - should restore original value
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 10 );
}

TEST( overridable_variable, observers_see_override_status_matching_value )
{
    Overridable_variable<int> variable( 10 );
    Override_status_observer observer( variable );

    ASSERT_TRUE( variable.set_override( 50 ) );
    variable = 20;
    variable.clear_override();

    ASSERT_EQ( observer.notifications, 2u );
    EXPECT_EQ( observer.values[0], 50 );
    EXPECT_TRUE( observer.statuses[0] );
    EXPECT_EQ( observer.values[1], 20 );
    EXPECT_FALSE( observer.statuses[1] );
}

TEST( overridable_variable, callback_runs_in_publishing_task )
{
    Overridable_variable<int> variable( 10 );
    Publishing_task_observer observer( variable );
    Owner_publication_context state { variable };
    TaskHandle_t owner_task = nullptr;

    ASSERT_EQ( xTaskCreate( owner_publication_task, "variable_owner", 1024u, &state, 1u, &owner_task ), pdPASS );

    bool owner_finished = wait_until_true( 200u, [&]() { return state.finished.load(); } );

    vTaskDelete( owner_task );
    ASSERT_TRUE( owner_finished );
    EXPECT_EQ( observer.notifications.load(), 1u );
    EXPECT_EQ( observer.observed_value.load(), 20 );
    EXPECT_EQ( observer.callback_task.load(), owner_task );

    TaskHandle_t overrider_task = xTaskGetCurrentTaskHandle();

    ASSERT_TRUE( variable.set_override( 50 ) );
    EXPECT_EQ( observer.notifications.load(), 2u );
    EXPECT_EQ( observer.observed_value.load(), 50 );
    EXPECT_EQ( observer.callback_task.load(), overrider_task );
}

TEST( overridable_variable, clear_waits_for_override_notification )
{
    Overridable_variable<int> variable( 10 );
    Concurrent_clear_context state { variable };
    Blocking_override_observer observer( variable, state );
    TaskHandle_t task_handle = nullptr;

    ASSERT_EQ( xTaskCreate( concurrent_clear_task, "override_clear", 1024u, &state, 1u, &task_handle ), pdPASS );

    EXPECT_TRUE( variable.set_override( 50 ) );
    EXPECT_TRUE( wait_until_true( 200u, [&]() { return state.completed.load(); } ) );
    EXPECT_TRUE( observer.clear_attempted_during_callback );
    EXPECT_FALSE( observer.clear_completed_during_callback );
    EXPECT_FALSE( variable.is_overridden() );
    EXPECT_EQ( variable.get(), 10 );

    vTaskDelete( task_handle );
}

TEST( overridable_variable, override_with_string )
{
    Overridable_variable<int> P_tested( 25 );
    Mock_variable_observer<int> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 100 ) );
    EXPECT_CALL( observer, changed( 25 ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 25 );

    // Override with string representation
    ASSERT_TRUE( P_tested.set_override( String_view( "100" ) ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 100 );

    // Clear override
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 25 );
}

TEST( overridable_variable, override_with_hex_string )
{
    Overridable_variable<uint32_t> P_tested( 10 );
    Mock_variable_observer<uint32_t> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 0xDEADBEEF ) );
    EXPECT_CALL( observer, changed( 10 ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 10 );

    // Override with hex string
    ASSERT_TRUE( P_tested.set_override( String_view( "0xDEADBEEF" ) ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 0xDEADBEEF );

    // Clear override
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 10 );
}

TEST( overridable_variable, override_with_boolean_string )
{
    Overridable_variable<bool> P_tested( false );
    Mock_variable_observer<bool> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( true ) );
    EXPECT_CALL( observer, changed( false ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, false );

    // Override with "true" string
    ASSERT_TRUE( P_tested.set_override( String_view( "true" ) ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, true );

    // Override with "false" string
    ASSERT_TRUE( P_tested.set_override( String_view( "false" ) ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, false );

    // Clear override
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, false );
}

TEST( overridable_variable, invalid_string_override )
{
    Overridable_variable<int> P_tested( 30 );

    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 30 );

    // Try to override with invalid string
    ASSERT_FALSE( P_tested.set_override( String_view( "invalid_number" ) ) );
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 30 );
}

TEST( overridable_variable, set_value_while_overridden )
{
    Overridable_variable<int> P_tested( 15 );
    Mock_variable_observer<int> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 100 ) );  // Override value
    EXPECT_CALL( observer, changed( 25 ) );   // Restored value after clear

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_EQ( P_tested, 15 );
    ASSERT_FALSE( P_tested.is_overridden() );

    // Override the variable
    ASSERT_TRUE( P_tested.set_override( 100 ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 100 );

    // Set new value while overridden - should not change current value but remember for later
    P_tested = 25;
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 100 );  // Still shows override value

    // Clear override - should show the new value that was set while overridden
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 25 );  // Shows the value set while overridden
}

TEST( overridable_variable, multiple_overrides )
{
    Overridable_variable<int> P_tested( 5 );
    Mock_variable_observer<int> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 20 ) );   // First override
    EXPECT_CALL( observer, changed( 30 ) );   // Second override
    EXPECT_CALL( observer, changed( 40 ) );   // Third override
    EXPECT_CALL( observer, changed( 5 ) );    // Clear override

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_EQ( P_tested, 5 );
    ASSERT_FALSE( P_tested.is_overridden() );

    // First override
    ASSERT_TRUE( P_tested.set_override( 20 ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 20 );

    // Second override (should replace first override)
    ASSERT_TRUE( P_tested.set_override( 30 ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 30 );

    // Third override
    ASSERT_TRUE( P_tested.set_override( 40 ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 40 );

    // Clear override - should restore original value
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 5 );
}

TEST( overridable_variable, clear_non_overridden )
{
    Overridable_variable<int> P_tested( 42 );

    ASSERT_EQ( P_tested, 42 );
    ASSERT_FALSE( P_tested.is_overridden() );

    // Clear override when not overridden - should be safe
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 42 );
}

TEST( overridable_variable, assignment_from_other_variable )
{
    Overridable_variable<int> P_source( 77 );
    Overridable_variable<int> P_target( 0 );
    Mock_variable_observer<int> target_observer( P_target );

    // **** EXPECTATIONS *********************************************************************************************************

    EXPECT_CALL( target_observer, changed( 77 ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_EQ( P_source, 77 );
    ASSERT_EQ( P_target, 0 );

    P_target = P_source;

    ASSERT_EQ( P_target, 77 );
    ASSERT_FALSE( P_target.is_overridden() );
}

TEST( overridable_variable, self_assignment_preserves_saved_value )
{
    Overridable_variable<int> variable( 3 );

    ASSERT_TRUE( variable.set_override( 9 ) );
    variable = variable;
    variable.clear_override();

    EXPECT_EQ( variable.get(), 3 );
}

TEST( overridable_variable, floating_point_override )
{
    Overridable_variable<float> P_tested( 3.14f );
    Mock_variable_observer<float> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 2.71f ) );
    EXPECT_CALL( observer, changed( 3.14f ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_EQ( P_tested, 3.14f );
    ASSERT_FALSE( P_tested.is_overridden() );

    // Override with float value
    ASSERT_TRUE( P_tested.set_override( 2.71f ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 2.71f );

    // Clear override
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 3.14f );
}

TEST( overridable_variable, floating_point_string_override )
{
    Overridable_variable<float> P_tested( 1.0f );
    Mock_variable_observer<float> observer( P_tested );

    // **** EXPECTATIONS *********************************************************************************************************

    InSequence seq;

    EXPECT_CALL( observer, changed( 2.5f ) );
    EXPECT_CALL( observer, changed( 1.0f ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    ASSERT_EQ( P_tested, 1.0f );
    ASSERT_FALSE( P_tested.is_overridden() );

    // Override with string representation of float
    ASSERT_TRUE( P_tested.set_override( String_view( "2.5" ) ) );
    ASSERT_TRUE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 2.5f );

    // Clear override
    P_tested.clear_override();
    ASSERT_FALSE( P_tested.is_overridden() );
    ASSERT_EQ( P_tested, 1.0f );
}
