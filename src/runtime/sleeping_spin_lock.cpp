// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/runtime/sleeping_spin_lock.h"

namespace SOMA
{
    void Sleeping_spin_lock::lock( void )
    {
        while( m_lock_flag.test_and_set( std::memory_order_acquire ) )
        {
            // Sleep instead of busy-waiting so a lower-priority owner can run.
            vTaskDelay( 1 );
        }
    }

    void Sleeping_spin_lock::unlock( void )
    {
        m_lock_flag.clear( std::memory_order_release );
    }
} // namespace SOMA
