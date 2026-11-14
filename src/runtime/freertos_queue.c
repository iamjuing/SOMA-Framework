// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"
#include "FreeRTOS/include/queue.h"

#include "freertos_queue_profile.h"

#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0

static void soma_queue_created( QueueHandle_t queue );
static void soma_queue_deleted( QueueHandle_t queue );

// Track regular queues without assigning names. The profiler reads names from
// FreeRTOS's registry when application code has registered one.
#undef traceQUEUE_CREATE
#define traceQUEUE_CREATE( queue ) soma_queue_created( ( QueueHandle_t ) ( queue ) )
#undef traceQUEUE_DELETE
#define traceQUEUE_DELETE( queue ) soma_queue_deleted( ( QueueHandle_t ) ( queue ) )

#endif // SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0

#include "FreeRTOS/queue.c"

#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0

static QueueHandle_t g_queues[SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT];

static void soma_queue_created( QueueHandle_t queue )
{
    if( ( ( Queue_t* ) queue )->ucQueueType == queueQUEUE_TYPE_BASE &&
        ( ( Queue_t* ) queue )->uxItemSize > 0u )
    {
        BaseType_t rv_added = pdFALSE;

        taskENTER_CRITICAL();

        for( UBaseType_t i = 0u; i < ( UBaseType_t ) SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT; ++i )
        {
            if( g_queues[i] == NULL )
            {
                g_queues[i] = queue;
                rv_added = pdTRUE;
                break;
            }
        }

        taskEXIT_CRITICAL();
        configASSERT( rv_added == pdTRUE );
    }
}

static void soma_queue_deleted( QueueHandle_t queue )
{
    if( ( ( Queue_t* ) queue )->ucQueueType == queueQUEUE_TYPE_BASE &&
        ( ( Queue_t* ) queue )->uxItemSize > 0u )
    {
        taskENTER_CRITICAL();

        for( UBaseType_t i = 0u; i < ( UBaseType_t ) SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT; ++i )
        {
            if( g_queues[i] == queue )
            {
                g_queues[i] = NULL;
                break;
            }
        }

        taskEXIT_CRITICAL();
    }
}

BaseType_t soma_queue_next_profile( uint32_t* p_out_cursor,
                                    SOMA_Queue_profile_t* out_profile )
{
    BaseType_t rv_found = pdFALSE;

    configASSERT( p_out_cursor != NULL );
    configASSERT( out_profile != NULL );

    if( ( p_out_cursor != NULL ) && ( out_profile != NULL ) )
    {
        taskENTER_CRITICAL();

        while( *p_out_cursor < ( uint32_t ) SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT )
        {
            const uint32_t slot = *p_out_cursor;
            QueueHandle_t queue = g_queues[slot];

            *p_out_cursor = slot + 1u;

            if( ( queue != NULL ) &&
                ( queue->ucQueueType == queueQUEUE_TYPE_BASE ) &&
                ( queue->uxItemSize > 0u ) )
            {
                out_profile->name = pcQueueGetName( queue );
                out_profile->messages_waiting = queue->uxMessagesWaiting;
                out_profile->length = queue->uxLength;
                out_profile->item_size_B = queue->uxItemSize;

                rv_found = pdTRUE;
                break;
            }
        }

        taskEXIT_CRITICAL();
    }

    return rv_found;
}

#endif // SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0
