// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/runtime/event_task.h"
#include "../internal.h"

namespace SOMA
{
    extern bool is_application_launched( void );

    Event_task::Event_task( String_view name,
                            Task::Priority_e priority,
                            B_task_memory& memory )
        : Task( name, priority, memory )
    {
    }

    void Event_task::register_timer( B_task_timer& timer )
    {
        ASSERT( is_application_launched() == false );
        m_timer_list.insert( timer );
    }

    uint32_t Event_task::register_event( B_task_event& event )
    {
        ASSERT( is_application_launched() == false );
        ASSERT( m_event_list.size() < 32u );

        uint32_t rv_notification_value = 1u << m_event_list.size();

        m_event_list.insert( event );

        return rv_notification_value;
    }

    B_task_timer* Event_task::get_earliest_timer( void )
    {
        B_task_timer* p_closest_timer = nullptr;

        for( B_task_timer& timer: m_timer_list )
        {
            if( timer.is_started() )
            {
                if( p_closest_timer )
                {
                    if( timer < ( *p_closest_timer ) )
                    {
                        p_closest_timer = &timer;
                    }
                }
                else
                {
                    p_closest_timer = &timer;
                }
            }
        }

        return p_closest_timer;
    }

    void Event_task::handle_events( uint32_t events_bit_mask )
    {
        for( B_task_event& event: m_event_list )
        {
            if( events_bit_mask & event.get_notification_value() )
            {
                event.handle();

                events_bit_mask &= ~event.get_notification_value();
            }

            if( events_bit_mask == 0u )
            // all events handled
            {
                break;
            }
        }
    }

    void Event_task::loop( void )
    {
        // provides the task startup notification to the observers
        notify();

        while( true )
        {
            uint32_t notification_value;
            bool timed_out = false;

            mp_earliest_timer = get_earliest_timer();

            if( mp_earliest_timer )
            {
                const TickType_t timeout_ticks = mp_earliest_timer->time_remaining_ticks();

                timed_out = Internal::wait_task_notification_ticks( notification_value,
                                                                    Internal::c_all_notification_bits,
                                                                    Internal::c_all_notification_bits,
                                                                    timeout_ticks );
            }
            else
            {
                wait_task_notification( notification_value,
                                        Internal::c_all_notification_bits,
                                        Internal::c_all_notification_bits );
            }

            if( timed_out )
            {
                ASSERT( mp_earliest_timer != nullptr );

                // notify the timer that it has run out
                mp_earliest_timer->run_out();
            }
            else
            {
                handle_events( notification_value );
            }
        }
    }
} // namespace SOMA
