// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_B_TASK_TIMER_H_
#define SOMA_B_TASK_TIMER_H_

#include <stddef.h>
#include <stdint.h>
#include <limits>

#include "soma/internal/port_traits.h"

#include "soma/containers/linked_list.h"
#include "soma/i_observable.h"

namespace SOMA
{
    class Event_task;
    class Observer;

    ///
    /// @brief Base class for a timer that allows tasks to run actions on internal time based events.
    ///        - one-shot or periodic
    ///        NOTE: Consider using Task_timer<T> template for simpler usage with automatic Observer management
    class B_task_timer: public I_observable
    {
    public:
        static constexpr uint32_t max_time_ms( void )
        {
            constexpr uint64_t max_interval_ticks = std::numeric_limits<Tick_t>::max() >> 1u;

            uint32_t rv_max_time_ms = std::numeric_limits<uint32_t>::max();

            if( max_interval_ticks <= std::numeric_limits<uint64_t>::max() / 1000u )
            {
                const uint64_t calculated_max_time_ms = ( max_interval_ticks * 1000u - 1u ) / Internal::c_tick_rate_hz;

                if( calculated_max_time_ms < rv_max_time_ms )
                {
                    rv_max_time_ms = static_cast<uint32_t>( calculated_max_time_ms );
                }
            }

            return rv_max_time_ms;
        }

        ///
        /// @brief ctor
        explicit B_task_timer( void );

        ///
        /// @brief ctor
        explicit B_task_timer( Event_task& event_task,
                               Observer& observer );

        ///
        /// @brief Compares with the provided timer.
        /// @param rhs right hand side timer to compare to
        /// @retval true, if this timer occures before the rhs timer
        ///         false otherwise
        bool operator<( const B_task_timer& rhs ) const;

        ///
        /// @brief Sets the timer to run out in specified number of milliseconds.
        /// @param timeout_ms number of milliseconds
        void start( uint32_t timeout_ms );

        ///
        /// @brief Sets the timer to run out continuously (until cancelled) with period of specified number of milliseconds.
        /// @param period_ms number of milliseconds
        void start_periodic( uint32_t period_ms );

        ///
        /// @brief Shall be called by a timer processor once the timer runs out.
        void run_out( void );

        ///
        /// @brief Cancels a running timer.
        /// @param period_ms number of milliseconds
        void cancel( void );

        ///
        /// @brief Retrieves whether the timer is periodic.
        /// @retval true, if the timer is periodic
        ///         false otherwise
        bool is_periodic( void ) const;

        ///
        /// @brief Retrieves whether the timer is started.
        /// @retval true, if the timer is started
        ///         false otherwise
        bool is_started( void ) const;

        /// @brief Retrieves the number of milliseconds remaining before timeout.
        uint32_t time_remaining_ms( void ) const;

    private:
        friend class Event_task;

        using Tick_t = Internal::Port_tick_t;

        STATIC_LINKED_LIST_ITEM( B_task_timer );

        bool m_is_started = false;
        bool m_is_periodic = false;

        Tick_t m_deadline_ticks = 0u;
        Tick_t m_period_ticks = 0u;

        Tick_t time_remaining_ticks( void ) const;
    };
} // namespace SOMA

#endif // B_TASK_TIMER_H_
