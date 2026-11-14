// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/assert.h"
#include "soma/runtime/task_memory.h"

namespace
{
    static_assert( SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES % sizeof( StackType_t ) == 0u,
                   "SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES must be divisible by sizeof(StackType_t)." );
    static_assert( ( SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES / sizeof( StackType_t ) ) <= UINT32_MAX,
                   "The configured idle-task stack is too large." );

    SOMA::Task_memory<SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES> g_idle_task_memory;
}

extern "C"
{
    void vApplicationIdleHook( void );

    void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char* pcTaskName );

    void vApplicationGetIdleTaskMemory( StaticTask_t** ppxIdleTaskTCBBuffer,
                                        StackType_t** ppxIdleTaskStackBuffer,
                                        uint32_t* pulIdleTaskStackSize );
}

void vApplicationIdleHook( void )
{
    #ifdef SOMA_CONFIG_IDLE_HOOK
    SOMA_CONFIG_IDLE_HOOK();
    #endif
}

void vApplicationGetIdleTaskMemory( StaticTask_t** ppxIdleTaskTCBBuffer,
                                    StackType_t** ppxIdleTaskStackBuffer,
                                    uint32_t* pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &g_idle_task_memory.TCB();
    *ppxIdleTaskStackBuffer = g_idle_task_memory.stack().storage();
    *pulIdleTaskStackSize = static_cast<uint32_t>( g_idle_task_memory.stack().size() );
}

void vApplicationStackOverflowHook( [[maybe_unused]] TaskHandle_t xTask,
                                    [[maybe_unused]] char* pcTaskName )
{
    SOMA_PANIC( "FreeRTOS task stack overflow" );
}
