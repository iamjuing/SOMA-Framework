// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/assert.h"
#include "soma/runtime/run.h"
#include "../internal.h"

namespace SOMA
{
    bool is_application_launched( void )
    {
        return Internal::g_application_launched.load();
    }

    void set_application_launched( bool launched )
    {
        Internal::g_application_launched.store( launched );
    }
}

namespace SOMA::Internal
{
    namespace
    {
        struct Startup_context_t
        {
            void ( *function ) ( void* );
        };

        void startup_task( void* parameter )
        {
            auto* context = static_cast<Startup_context_t*>( parameter );
            ASSERT( context != nullptr );
            context->function( nullptr );
            vTaskDelete( nullptr );
        }
    }

    [[noreturn]] void run_after_heap_initialization( void ( *startup_function ) ( void* ),
                                                     B_task_memory& startup_task_memory )
    {
        ASSERT( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED );
        ASSERT( startup_function != nullptr );

        Startup_context_t startup_context { startup_function };

        ASSERT( xTaskCreateStatic( startup_task,
                                   "startup",
                                   static_cast<configSTACK_DEPTH_TYPE>( startup_task_memory.stack().size() ),
                                   &startup_context,
                                   tskIDLE_PRIORITY,
                                   startup_task_memory.stack().storage(),
                                   &startup_task_memory.TCB() ) != nullptr );

        ASSERT( uxTaskGetNumberOfTasks() > 0u );

        vTaskStartScheduler();

        ASSERT( false );

        for( ;; )
        {
        }
    }
}
