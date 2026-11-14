// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cstdint>

#include "soma/runtime/b_task_timer.h"
#include "soma/runtime/event_task.h"
#include "../internal.h"

namespace SOMA
{
    B_task_timer::B_task_timer( Event_task& event_task,
                                Observer& observer )
    {
        event_task.register_timer( *this );
        subscribe( observer );
    }

    B_task_timer::B_task_timer( void )
    {
        // intentionally empty
    }

    bool B_task_timer::operator<( const B_task_timer& rhs ) const
    {
        const bool rv_before = Internal::ticks_before( static_cast<TickType_t>( m_deadline_ticks ),
                                                       static_cast<TickType_t>( rhs.m_deadline_ticks ) );

        return rv_before;
    }

    void B_task_timer::start( uint32_t timeout_ms )
    {
        ASSERT( IS_IN_STATIC_LIST() );

        ASSERT( timeout_ms <= max_time_ms() );

        const TickType_t timeout_ticks = Internal::milliseconds_to_ticks( timeout_ms );

        const TickType_t guarded_timeout_ticks = timeout_ticks + 1u;
        const TickType_t deadline_ticks = Internal::tick_count() + guarded_timeout_ticks;

        m_deadline_ticks = static_cast<Tick_t>( deadline_ticks );

        m_is_started = true;
        m_is_periodic = false;
    }

    void B_task_timer::start_periodic( uint32_t period_ms )
    {
        ASSERT( IS_IN_STATIC_LIST() );

        ASSERT( ( period_ms > 0u ) && ( period_ms <= max_time_ms() ) );

        const TickType_t period_ticks = Internal::milliseconds_to_ticks( period_ms );

        const TickType_t guarded_period_ticks = period_ticks + 1u;
        const TickType_t deadline_ticks = Internal::tick_count() + guarded_period_ticks;

        m_period_ticks = static_cast<Tick_t>( guarded_period_ticks );
        m_deadline_ticks = static_cast<Tick_t>( deadline_ticks );

        m_is_started = true;
        m_is_periodic = true;
    }

    void B_task_timer::run_out( void )
    {
        if( is_periodic() )
        // periodic timer must shift its new end by the period
        {
            const TickType_t deadline_ticks = static_cast<TickType_t>( m_deadline_ticks );
            const TickType_t period_ticks = static_cast<TickType_t>( m_period_ticks );

            m_deadline_ticks = static_cast<Tick_t>( deadline_ticks + period_ticks );
        }
        else
        // one-shot stops now
        {
            cancel();
        }

        notify();
    }

    void B_task_timer::cancel( void )
    {
        m_is_started = false;
        m_is_periodic = false;
    }

    bool B_task_timer::is_periodic( void ) const
    {
        return m_is_periodic;
    }

    bool B_task_timer::is_started( void ) const
    {
        return m_is_started;
    }

    uint32_t B_task_timer::time_remaining_ms( void ) const
    {
        const TickType_t remaining_ticks = static_cast<TickType_t>( time_remaining_ticks() );
        const uint32_t rv_remaining_ms = Internal::ticks_to_milliseconds( remaining_ticks );

        return rv_remaining_ms;
    }

    B_task_timer::Tick_t B_task_timer::time_remaining_ticks( void ) const
    {
        const TickType_t remaining_ticks = Internal::ticks_remaining( static_cast<TickType_t>( m_deadline_ticks ) );
        const Tick_t rv_remaining_ticks = static_cast<Tick_t>( remaining_ticks );

        return rv_remaining_ticks;
    }
} // namespace SOMA
