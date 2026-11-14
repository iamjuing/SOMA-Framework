// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"

#include "soma/assert.h"
#include "../internal.h"
#include "soma/runtime/binary_semaphore.h"
#include "soma/runtime/task.h"

namespace SOMA
{
    Binary_semaphore::Binary_semaphore( Initial_state_e initial_state )
    {
        m_handle = xSemaphoreCreateBinaryStatic( &m_buffer );

        if( initial_state == Initial_state_e::free )
        {
            ASSERT( xSemaphoreGive( m_handle ) == pdPASS );
        }
    }

    bool Binary_semaphore::try_take( void ) const
    {
        bool rv_taken = false;

        if( Task::in_ISR() )
        {
            auto yield = pdFALSE;

            if( xSemaphoreTakeFromISR( m_handle, &yield ) == pdPASS )
            {
                rv_taken = true;
                portYIELD_FROM_ISR( yield );
            }
        }
        else if( xSemaphoreTake( m_handle, 0 ) == pdPASS )
        {
            rv_taken = true;
        }

        return rv_taken;
    }

    void Binary_semaphore::take( void ) const
    {
        ASSERT( Task::in_ISR() == false );

        while( xSemaphoreTake( m_handle,
                               static_cast<TickType_t>( Internal::milliseconds_to_ticks( mc_interruption_time_ms ) ) ) == pdFAIL )
        {
            // intentionally empty
        }
    }

    bool Binary_semaphore::take( uint32_t time_ms ) const
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

    void Binary_semaphore::give( void ) const
    {
        if( Task::in_ISR() )
        {
            auto yield = pdFALSE;

            // Giving an already-given semaphore is intentionally harmless.
            xSemaphoreGiveFromISR( m_handle, &yield );
            portYIELD_FROM_ISR( yield );
        }
        else
        {
            xSemaphoreGive( m_handle );
        }
    }
} // namespace SOMA
