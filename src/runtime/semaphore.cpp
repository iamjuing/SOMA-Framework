// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"

#include "soma/assert.h"
#include "../internal.h"
#include "soma/runtime/semaphore.h"
#include "soma/runtime/task.h"

namespace SOMA
{
    Semaphore::Semaphore( uint32_t max_count,
                          uint32_t initial_count )
    {
        m_handle = xSemaphoreCreateCountingStatic( max_count, initial_count, &m_buffer );
    }

    bool Semaphore::try_take( void ) const
    {
        bool rv_taken;

        if( Task::in_ISR() )
        {
            auto yield = pdFALSE;

            rv_taken = xSemaphoreTakeFromISR( m_handle, &yield ) == pdPASS;

            if( rv_taken )
            {
                portYIELD_FROM_ISR( yield );
            }
        }
        else
        {
            rv_taken = xSemaphoreTake( m_handle, 0 ) == pdPASS;
        }

        return rv_taken;
    }

    void Semaphore::take( void ) const
    {
        ASSERT( Task::in_ISR() == false );

        while( xSemaphoreTake( m_handle,
                               static_cast<TickType_t>( Internal::milliseconds_to_ticks( mc_interruption_time_ms ) ) ) == pdFAIL )
        {
            // intentionally empty
        }
    }

    bool Semaphore::take( uint32_t time_ms ) const
    {
        bool rv_taken = false;
        uint32_t remaining_time_ms = time_ms;

        ASSERT( Task::in_ISR() == false );

        // Periodic retries let centralized watchdog supervision observe task activity.
        while( ( remaining_time_ms > 0u ) && ( rv_taken == false ) )
        {
            const uint32_t chunk_time_ms = ( remaining_time_ms > mc_interruption_time_ms ) ?
                                           mc_interruption_time_ms : remaining_time_ms;

            if( xSemaphoreTake( m_handle,
                                static_cast<TickType_t>( Internal::milliseconds_to_ticks( chunk_time_ms ) ) ) == pdPASS )
            {
                rv_taken = true;
            }
            else
            {
                remaining_time_ms -= chunk_time_ms;
            }
        }

        return rv_taken;
    }

    void Semaphore::give( void ) const
    {
        if( Task::in_ISR() )
        {
            auto yield = pdFALSE;

            // Giving at the maximum count is intentionally harmless.
            xSemaphoreGiveFromISR( m_handle, &yield );
            portYIELD_FROM_ISR( yield );
        }
        else
        {
            xSemaphoreGive( m_handle );
        }
    }
} // namespace SOMA
