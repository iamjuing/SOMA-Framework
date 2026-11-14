// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <limits>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "internal.h"
#include "soma/time.h"

namespace SOMA
{
    namespace Internal
    {
        TickType_t milliseconds_to_ticks( uint32_t time_ms )
        {
            const uint64_t ticks = ( static_cast<uint64_t>( time_ms ) * configTICK_RATE_HZ ) / 1000u;
            const TickType_t rv_ticks = static_cast<TickType_t>( ticks );

            configASSERT( ticks <= std::numeric_limits<TickType_t>::max() );

            return rv_ticks;
        }

        uint32_t ticks_to_milliseconds( TickType_t ticks )
        {
            const uint64_t ticks_64 = static_cast<uint64_t>( ticks );
            const uint64_t whole_seconds = ticks_64 / configTICK_RATE_HZ;
            const uint64_t fractional_ticks = ticks_64 % configTICK_RATE_HZ;
            const uint64_t fractional_ms = ( ( fractional_ticks * 1000u ) + configTICK_RATE_HZ - 1u ) /
                                           configTICK_RATE_HZ;
            const uint64_t max_time_ms = std::numeric_limits<uint32_t>::max();
            uint32_t rv_time_ms = static_cast<uint32_t>( max_time_ms );

            if( fractional_ms <= max_time_ms &&
                whole_seconds <= ( ( max_time_ms - fractional_ms ) / 1000u ) )
            {
                rv_time_ms = static_cast<uint32_t>( ( whole_seconds * 1000u ) + fractional_ms );
            }

            return rv_time_ms;
        }

        TickType_t tick_count( void )
        {
            const TickType_t rv_tick_count = xTaskGetTickCount();

            return rv_tick_count;
        }

        TickType_t ticks_remaining( TickType_t deadline )
        {
            const TickType_t ticks_to_deadline = deadline - tick_count();
            TickType_t rv_ticks_remaining = 0;

            if( ( ticks_to_deadline > 0u ) && ( ticks_to_deadline <= max_interval<TickType_t>() ) )
            {
                rv_ticks_remaining = ticks_to_deadline;
            }

            return rv_ticks_remaining;
        }
    } // namespace Internal

    namespace Time
    {
        int64_t Stamp::operator-( const Stamp& rhs ) const
        {
            const uint32_t time_difference_ms = m_time_ms - rhs.m_time_ms;
            int64_t rv_difference_ms = static_cast<int64_t>( time_difference_ms );

            if( time_difference_ms > max_interval_ms() )
            {
                rv_difference_ms -= ( static_cast<int64_t>( UINT32_MAX ) + 1 );
            }

            return rv_difference_ms;
        }

        Stamp& Stamp::operator+( uint32_t rhs_time_ms )
        {
            m_time_ms += rhs_time_ms;

            return *this;
        }

        void Stamp::operator+=( uint32_t time_ms )
        {
            m_time_ms += time_ms;
        }

        bool Stamp::operator<( const Stamp& rhs ) const
        {
            const bool rv_before = ( ( *this ) - rhs ) < 0;

            return rv_before;
        }

        bool Stamp::operator>( const Stamp& rhs ) const
        {
            const bool rv_after = rhs < ( *this );

            return rv_after;
        }

        uint32_t Stamp::get_remaining_ms( void ) const
        {
            const int64_t time_difference_ms = *this - Stamp::now();
            uint32_t rv_remaining_ms = 0u;

            if( time_difference_ms > 0 )
            {
                rv_remaining_ms = static_cast<uint32_t>( time_difference_ms );
            }

            return rv_remaining_ms;
        }

        bool Stamp::passed( void ) const
        {
            const bool rv_passed = get_remaining_ms() == 0u;

            return rv_passed;
        }

        Stamp Stamp::now( void )
        {
            const Stamp rv_stamp { uptime_ms() };

            return rv_stamp;
        }

        uint32_t uptime_ms( void )
        {
            TimeOut_t tick_state;
            vTaskSetTimeOutState( &tick_state );

            uint64_t total_ticks = static_cast<uint64_t>( tick_state.xTimeOnEntering );

            if constexpr( std::numeric_limits<TickType_t>::digits < 64 )
            {
                const uint64_t ticks_per_overflow = static_cast<uint64_t>( std::numeric_limits<TickType_t>::max() ) + 1u;
                const uint64_t overflow_count = static_cast<uint32_t>( tick_state.xOverflowCount );

                total_ticks += overflow_count * ticks_per_overflow;
            }

            const uint64_t whole_seconds = total_ticks / configTICK_RATE_HZ;
            const uint64_t fractional_ticks = total_ticks % configTICK_RATE_HZ;
            const uint32_t whole_seconds_ms = static_cast<uint32_t>( whole_seconds ) * 1000u;
            const uint32_t fractional_ms = static_cast<uint32_t>( ( fractional_ticks * 1000u ) / configTICK_RATE_HZ );
            const uint32_t rv_uptime_ms = whole_seconds_ms + fractional_ms;

            return rv_uptime_ms;
        }
    } // namespace Time
} // namespace SOMA
