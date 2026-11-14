// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"

#include "soma/assert.h"
#include "../internal.h"
#include "soma/runtime/mutex.h"
#include "soma/runtime/task.h"

namespace SOMA
{
    Mutex::Mutex( void )
    {
        m_handle = xSemaphoreCreateMutexStatic( &m_buffer );
    }

    bool Mutex::is_locked( void ) const
    {
        return Task::in_ISR() ?
               ( uxSemaphoreGetCountFromISR( m_handle ) == 0u ) :
               ( uxSemaphoreGetCount( m_handle ) == 0u );
    }

    bool Mutex::try_lock( void ) const
    {
        // FreeRTOS mutexes support task context only and implement priority inheritance.
        ASSERT( Task::in_ISR() == false );

        return xSemaphoreTake( m_handle, 0 ) == pdPASS;
    }

    void Mutex::lock( void ) const
    {
        ASSERT( Task::in_ISR() == false );

        while( xSemaphoreTake( m_handle,
                               static_cast<TickType_t>( Internal::milliseconds_to_ticks( mc_interruption_time_ms ) ) ) == pdFAIL )
        {
            // intentionally empty
        }
    }

    bool Mutex::lock( uint32_t time_ms ) const
    {
        bool rv_locked = false;
        uint32_t remaining_time_ms = time_ms;

        ASSERT( Task::in_ISR() == false );

        while( ( remaining_time_ms > 0u ) && ( rv_locked == false ) )
        {
            const uint32_t chunk_time_ms = ( remaining_time_ms > mc_interruption_time_ms ) ?
                                           mc_interruption_time_ms : remaining_time_ms;

            if( xSemaphoreTake( m_handle,
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

    void Mutex::unlock( void ) const
    {
        ASSERT( Task::in_ISR() == false );
        ASSERT( xSemaphoreGive( m_handle ) == pdPASS );
    }
} // namespace SOMA
