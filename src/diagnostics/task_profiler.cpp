// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cstring>
#include <type_traits>

#include "soma/diagnostics/task_profiler.h"
#include "soma/runtime/scheduler_suspension_guard.h"

extern "C"
{
    UBaseType_t uxTaskGetActiveTaskCount( void );
    BaseType_t xTaskGetNextSystemState( TaskStatus_t* p_task_status,
                                        char* p_task_name,
                                        UBaseType_t* p_last_task_number,
                                        BaseType_t* p_has_last_task_number );
    configRUN_TIME_COUNTER_TYPE ulTaskGetTotalRunTime( void );
}

static_assert( std::is_same_v<configRUN_TIME_COUNTER_TYPE, uint32_t>,
               "SOMA requires 32-bit FreeRTOS run-time counters." );

namespace SOMA::Internal
{
    uint32_t task_profiler_total_runtime( void )
    {
        return static_cast<uint32_t>( ulTaskGetTotalRunTime() );
    }

    size_t task_profiler_active_task_count( void )
    {
        return static_cast<size_t>( uxTaskGetActiveTaskCount() );
    }

    bool task_profiler_next_task( Task_profiler_cursor_t& cursor,
                                  char* task_name,
                                  size_t task_name_capacity,
                                  uint32_t& task_number,
                                  int& task_state,
                                  uint32_t& base_priority,
                                  uint32_t& runtime_counter )
    {
        configASSERT( task_name != nullptr );
        configASSERT( task_name_capacity >= configMAX_TASK_NAME_LEN );

        TaskStatus_t task_status {};
        char native_task_name[configMAX_TASK_NAME_LEN] {};
        UBaseType_t last_task_number = static_cast<UBaseType_t>( cursor.last_task_number );
        BaseType_t has_last_task_number = cursor.has_last_task_number ? pdTRUE : pdFALSE;
        BaseType_t task_found;

        {
            Scheduler_suspension_guard scheduler_suspension;

            task_found = xTaskGetNextSystemState( &task_status,
                                                  native_task_name,
                                                  &last_task_number,
                                                  &has_last_task_number );

            if( ( task_found == pdTRUE ) && ( task_status.eCurrentState == eSuspended ) )
            {
                task_status.eCurrentState = eTaskGetState( task_status.xHandle );
            }
        }

        const bool rv_task_found = task_found != pdFALSE;

        if( rv_task_found )
        {
            cursor.last_task_number = static_cast<uint32_t>( last_task_number );
            cursor.has_last_task_number = has_last_task_number != pdFALSE;
            task_number = static_cast<uint32_t>( task_status.xTaskNumber );
            task_state = static_cast<int>( task_status.eCurrentState );
            base_priority = static_cast<uint32_t>( task_status.uxBasePriority );
            runtime_counter = static_cast<uint32_t>( task_status.ulRunTimeCounter );
            std::memcpy( task_name, native_task_name, std::min( task_name_capacity, sizeof( native_task_name ) ) );
            task_name[task_name_capacity - 1u] = '\0';
        }

        return rv_task_found;
    }
}
