// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_CONFIG_WRAPPER_H_
#define SOMA_CONFIG_WRAPPER_H_

#include <soma_config.h>

#if !defined( SOMA_CONFIG_CPU_CLOCK_HZ ) || SOMA_CONFIG_CPU_CLOCK_HZ <= 0
    #error Define SOMA_CONFIG_CPU_CLOCK_HZ in soma_config.h as a positive clock frequency.
#endif

#if !defined( SOMA_CONFIG_TICK_RATE_HZ ) || SOMA_CONFIG_TICK_RATE_HZ <= 0
    #error Define SOMA_CONFIG_TICK_RATE_HZ in soma_config.h as a positive tick frequency.
#endif

#if !defined( SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES ) || SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES <= 0
    #error Define SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES in soma_config.h as a positive byte count.
#endif

#if !defined( SOMA_CONFIG_TICK_TYPE_WIDTH_BITS ) || \
    ( SOMA_CONFIG_TICK_TYPE_WIDTH_BITS != 16 && SOMA_CONFIG_TICK_TYPE_WIDTH_BITS != 32 && SOMA_CONFIG_TICK_TYPE_WIDTH_BITS != 64 )
    #error Define SOMA_CONFIG_TICK_TYPE_WIDTH_BITS in soma_config.h as 16, 32, or 64.
#endif

// default values for optional configuration parameters
#ifndef SOMA_CONFIG_SYNC_WAIT_CHUNK_MS
    #define SOMA_CONFIG_SYNC_WAIT_CHUNK_MS 100u
#endif

#ifndef SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT
    #define SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT 0u
#endif

#endif // SOMA_CONFIG_WRAPPER_H_
