// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/runtime/event_task.h"
#include "soma/runtime/task_timer.h"
#include "soma/runtime/task_event.h"
#include "soma/io/i_output_stream.h"
#include "soma/runtime/task_memory.h"
#include "soma/time.h"

using namespace SOMA;

class My_observable: public I_observable
{
public:
    void notify( void )
    {
        I_observable::notify();
    }
};

class Test_task: public Event_task
{
public:
    Test_task( const char* p_name,
               Task::Priority_e priority,
               B_task_memory& storage )
        : Event_task( p_name, priority, storage ),
          m_timer_1( *this, *this, &Test_task::callback_timer_1 ),
          m_timer_2( *this, *this, &Test_task::callback_timer_2 ),
          m_timer_3( *this, *this, &Test_task::callback_timer_3 ),
          m_event_1( *this, m_observable_1, *this, &Test_task::callback_event_1 ),
          m_event_2( *this, m_observable_2, *this, &Test_task::callback_event_2 ),
          m_event_3( *this, m_observable_3, *this, &Test_task::callback_event_3 ),
          m_start_timers_event( *this, m_start_timers_signal, *this, &Test_task::on_start_timers )
    {
    }

    My_observable m_observable_1;
    My_observable m_observable_2;
    My_observable m_observable_3;

    Task_timer<Test_task> m_timer_1;
    Task_timer<Test_task> m_timer_2;
    Task_timer<Test_task> m_timer_3;

    // Signal to trigger timer starts from within task context
    My_observable m_start_timers_signal;

    Task_event<Test_task> m_event_1;
    Task_event<Test_task> m_event_2;
    Task_event<Test_task> m_event_3;

    Task_event<Test_task> m_start_timers_event;

private:
    virtual void callback_timer_1( void ) = 0;
    virtual void callback_timer_2( void ) = 0;
    virtual void callback_timer_3( void ) = 0;

    virtual void callback_event_1( void ) = 0;
    virtual void callback_event_2( void ) = 0;
    virtual void callback_event_3( void ) = 0;

    virtual void on_start_timers( void ) = 0;
};

class Mock_test_class: public Test_task
{
public:
    Mock_test_class( const char* p_name,
                     Task::Priority_e priority,
                     B_task_memory& storage )
        : Test_task( p_name, priority, storage )
    {
        // intentionally empty
    }

    MOCK_METHOD( void, callback_timer_1, ( ), ( override ) );
    MOCK_METHOD( void, callback_timer_2, ( ), ( override ) );
    MOCK_METHOD( void, callback_timer_3, ( ), ( override ) );

    MOCK_METHOD( void, callback_event_1, ( ), ( override ) );
    MOCK_METHOD( void, callback_event_2, ( ), ( override ) );
    MOCK_METHOD( void, callback_event_3, ( ), ( override ) );

    MOCK_METHOD( void, on_start_timers, ( ), ( override ) );
};

TEST( event_task, events )
{
    Task_memory<1000> m_task_storage;
    NiceMock<Mock_test_class> mock_test_class( "test_task", Task::Priority_e::high, m_task_storage );

    mock_test_class.start();

    // **** EXPECTATIONS *********************************************************************************************************

    std::atomic_bool end_condition_reached = false;

    EXPECT_CALL( mock_test_class, callback_event_1() )
        .Times( 1 );

    EXPECT_CALL( mock_test_class, callback_event_2() )
        .Times( 1 );

    EXPECT_CALL( mock_test_class, callback_event_3() )
        .WillOnce( Invoke( [&]( ) { end_condition_reached = true; } ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    // this is a multi-threaded test, need to wait until the task is started
    ASSERT_EQ_IN_TIME( mock_test_class.is_started(),
                       true,
                       5000 );

    mock_test_class.m_observable_1.notify();
    mock_test_class.m_observable_2.notify();
    mock_test_class.m_observable_3.notify();

    // this is a multi-threaded test, we need to wait for an end condition
    ASSERT_EQ_IN_TIME( end_condition_reached,
                       true,
                       5000 );
}

TEST( event_task, timers )
{
    Task_memory<1000> m_task_storage;
    NiceMock<Mock_test_class> mock_test_class( "test_task", Task::Priority_e::high, m_task_storage );

    mock_test_class.start();

    // **** EXPECTATIONS *********************************************************************************************************

    std::atomic_bool end_condition_reached = false;

    // Expect the task to start timers from within its own context
    EXPECT_CALL( mock_test_class, on_start_timers() )
        .WillOnce( Invoke( [&]( ) {
            // Start timers from within the task context
            mock_test_class.m_timer_1.start( 200 );
            mock_test_class.m_timer_2.start( 500 );
            mock_test_class.m_timer_3.start( 1000 );
        } ) );

    EXPECT_CALL( mock_test_class, callback_timer_1() )
        .Times( 1 );

    EXPECT_CALL( mock_test_class, callback_timer_2() )
        .Times( 1 );

    EXPECT_CALL( mock_test_class, callback_timer_3() )
        .WillOnce( Invoke( [&]( ) { end_condition_reached = true; } ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    // this is a multi-threaded test, need to wait until the task is started
    ASSERT_EQ_IN_TIME( mock_test_class.is_started(),
                       true,
                       5000 );

    // Trigger the timer starts from within the task context
    mock_test_class.m_start_timers_signal.notify();

    // this is a multi-threaded test, we need to wait for an end condition
    ASSERT_EQ_IN_TIME( end_condition_reached,
                       true,
                       5000 );
}

TEST( event_task, timer_cancel )
{
    Task_memory<1000> m_task_storage;
    NiceMock<Mock_test_class> mock_test_class( "test_task", Task::Priority_e::high, m_task_storage );

    mock_test_class.start();

    // **** EXPECTATIONS *********************************************************************************************************

    std::atomic_bool end_condition_reached = false;

    EXPECT_CALL( mock_test_class, on_start_timers() )
        .WillOnce( Invoke( [&]( ) {
            // Start timers from within the task context
            mock_test_class.m_timer_3.start( 1000 );
            mock_test_class.m_timer_2.start( 500 );
            mock_test_class.m_timer_1.start( 200 );
        } ) );

    EXPECT_CALL( mock_test_class, callback_timer_1() )
        .WillOnce( Invoke( [&]( ) { mock_test_class.m_timer_2.cancel(); } ) );

    EXPECT_CALL( mock_test_class, callback_timer_2() )
        .Times( 0 );

    EXPECT_CALL( mock_test_class, callback_timer_3() )
        .WillOnce( Invoke( [&]( ) { end_condition_reached = true; } ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    // this is a multi-threaded test, need to wait until the task is started
    ASSERT_EQ_IN_TIME( mock_test_class.is_started(),
                       true,
                       5000 );

    // Trigger the timer starts from within the task context
    mock_test_class.m_start_timers_signal.notify();

    // this is a multi-threaded test, we need to wait for an end condition
    ASSERT_EQ_IN_TIME( end_condition_reached,
                       true,
                       5000 );
}

TEST( event_task, mixed )
{
    Task_memory<1000> m_task_storage;
    NiceMock<Mock_test_class> mock_test_class( "test_task", Task::Priority_e::high, m_task_storage );

    mock_test_class.start();

    // **** EXPECTATIONS *********************************************************************************************************

    std::atomic_bool end_condition_reached = false;

    EXPECT_CALL( mock_test_class, callback_timer_1() )
        .WillOnce( Invoke( [&]( ) { mock_test_class.m_observable_2.notify(); } ) );

    EXPECT_CALL( mock_test_class, callback_timer_2() )
        .WillOnce( Invoke( [&]( ) { mock_test_class.m_observable_3.notify(); } ) );

    EXPECT_CALL( mock_test_class, callback_timer_3() )
        .WillOnce( Invoke( [&]( ) { end_condition_reached = true; } ) );


    EXPECT_CALL( mock_test_class, callback_event_1() )
        .WillOnce( Invoke( [&]( ) { mock_test_class.m_timer_1.start( 100 ); } ) );

    EXPECT_CALL( mock_test_class, callback_event_2() )
        .WillOnce( Invoke( [&]( ) { mock_test_class.m_timer_2.start( 100 ); } ) );

    EXPECT_CALL( mock_test_class, callback_event_3() )
        .WillOnce( Invoke( [&]( ) { mock_test_class.m_timer_3.start( 100 ); } ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    // this is a multi-threaded test, need to wait until the task is started
    ASSERT_EQ_IN_TIME( mock_test_class.is_started(),
                       true,
                       5000 );

    mock_test_class.m_observable_1.notify();

    // this is a multi-threaded test, we need to wait for an end condition
    ASSERT_EQ_IN_TIME( end_condition_reached,
                       true,
                       5000 );
}


TEST( event_task, timer_periodic )
{
    Task_memory<1000> m_task_storage;
    NiceMock<Mock_test_class> mock_test_class( "test_task", Task::Priority_e::high, m_task_storage );

    mock_test_class.start();

    // **** EXPECTATIONS *********************************************************************************************************

    std::atomic_bool end_condition_reached = false;

    EXPECT_CALL( mock_test_class, on_start_timers() )
        .WillOnce( Invoke( [&]( ) {
            // Start periodic timers from within the task context
            mock_test_class.m_timer_1.start_periodic( 20 );
            mock_test_class.m_timer_2.start_periodic( 10 );
            mock_test_class.m_timer_3.start_periodic( 40 );
        } ) );

    EXPECT_CALL( mock_test_class, callback_timer_1() )
        .WillRepeatedly( Invoke( [&]( ) {
                                     static unsigned int call_cnt = 0u;
                                     ++call_cnt;
                                     if( call_cnt > 10u )
                                     {
                                         end_condition_reached = true;
                                     }
                                 } ) );

    EXPECT_CALL( mock_test_class, callback_timer_2() )
        .WillRepeatedly( Invoke( [&]( ) {
                                     static unsigned int call_cnt = 0u;
                                     ++call_cnt;
                                     if( call_cnt > 10u )
                                     {
                                         end_condition_reached = true;
                                     }
                                 } ) );

    EXPECT_CALL( mock_test_class, callback_timer_3() )
        .WillRepeatedly( Invoke( [&]( ) {
                                     static unsigned int call_cnt = 0u;
                                     ++call_cnt;
                                     if( call_cnt > 10u )
                                     {
                                         end_condition_reached = true;
                                     }
                                 } ) );

    // **** TEST SEQUENCE ********************************************************************************************************

    // this is a multi-threaded test, need to wait until the task is started
    ASSERT_EQ_IN_TIME( mock_test_class.is_started(),
                       true,
                       5000 );

    // Trigger the timer starts from within the task context
    mock_test_class.m_start_timers_signal.notify();

    // this is a multi-threaded test, we need to wait for an end condition
    ASSERT_EQ_IN_TIME( end_condition_reached,
                       true,
                       5000 );
}

TEST( event_task, periodic_timer_uses_requested_period )
{
    constexpr uint32_t period_ms = 25u;
    constexpr unsigned int callback_count_to_measure = 40u;

    Task_memory<1000> m_task_storage;
    NiceMock<Mock_test_class> mock_test_class( "test_task", Task::Priority_e::high, m_task_storage );

    mock_test_class.start();

    std::atomic_bool end_condition_reached = false;
    uint32_t first_callback_ms = 0u;
    uint32_t last_callback_ms = 0u;
    unsigned int callback_count = 0u;

    EXPECT_CALL( mock_test_class, on_start_timers() )
        .WillOnce( Invoke( [&]() {
            mock_test_class.m_timer_1.start_periodic( period_ms );
        } ) );

    EXPECT_CALL( mock_test_class, callback_timer_1() )
        .WillRepeatedly( Invoke( [&]() {
            ++callback_count;

            if( callback_count == 1u )
            {
                first_callback_ms = Time::uptime_ms();
            }
            else if( callback_count == callback_count_to_measure + 1u )
            {
                last_callback_ms = Time::uptime_ms();
                end_condition_reached = true;
            }
        } ) );

    ASSERT_EQ_IN_TIME( mock_test_class.is_started(), true, 5000 );
    mock_test_class.m_start_timers_signal.notify();

    ASSERT_EQ_IN_TIME( end_condition_reached, true, 5000 );

    const uint32_t elapsed_ms = last_callback_ms - first_callback_ms;
    const uint32_t expected_elapsed_ms = period_ms * callback_count_to_measure;
    EXPECT_LE( elapsed_ms, expected_elapsed_ms + 20u );
}
