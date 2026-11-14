// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"

#include "soma/assert.h"
#include "../internal.h"
#include "soma/runtime/recursive_mutex.h"
#include "soma/runtime/task.h"

namespace SOMA
{
    Recursive_mutex::Recursive_mutex( void )
    {
        m_handle = xSemaphoreCreateRecursiveMutexStatic( &m_buffer );
    }

    bool Recursive_mutex::try_lock( void )
    {
        return lock( 0u );
    }

    void Recursive_mutex::lock( void )
    {
        ASSERT( Task::in_ISR() == false );

        while( xSemaphoreTakeRecursive( m_handle,
                                        static_cast<TickType_t>( Internal::milliseconds_to_ticks( mc_interruption_time_ms ) ) ) ==
               pdFAIL )
        {
            // intentionally empty
        }
    }

    bool Recursive_mutex::lock( uint32_t time_ms )
    {
        bool rv_locked = false;
        uint32_t remaining_time_ms = time_ms;

        ASSERT( Task::in_ISR() == false );

        while( ( remaining_time_ms > 0u ) && ( rv_locked == false ) )
        {
            const uint32_t chunk_time_ms = ( remaining_time_ms > mc_interruption_time_ms ) ?
                                           mc_interruption_time_ms : remaining_time_ms;

            if( xSemaphoreTakeRecursive( m_handle,
                                         static_cast<TickType_t>( Internal::milliseconds_to_ticks( chunk_time_ms ) ) ) == pdPASS )
            {
                rv_locked = true;
            }
            else
            {
                remaining_time_ms -= chunk_time_ms;
            }
        }

        return rv_locked;
    }

    void Recursive_mutex::unlock( void )
    {
        ASSERT( Task::in_ISR() == false );
        ASSERT( xSemaphoreGiveRecursive( m_handle ) == pdPASS );
    }
} // namespace SOMA
