// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_FREERTOS_QUEUE_PROFILE_H_
#define SOMA_FREERTOS_QUEUE_PROFILE_H_

#include <stdint.h>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0

typedef struct SOMA_Queue_profile_t
{
    const char* name;
    UBaseType_t messages_waiting;
    UBaseType_t length;
    UBaseType_t item_size_B;
} SOMA_Queue_profile_t;

BaseType_t soma_queue_next_profile( uint32_t* cursor,
                                    SOMA_Queue_profile_t* out_profile );

#endif // SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT > 0

#ifdef __cplusplus
}
#endif

#endif // SOMA_FREERTOS_QUEUE_PROFILE_H_
