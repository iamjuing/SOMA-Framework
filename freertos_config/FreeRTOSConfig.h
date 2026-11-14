// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

// FreeRTOS configuration composition point owned by SOMA.

#ifndef SOMA_FREERTOS_CONFIG_H
#define SOMA_FREERTOS_CONFIG_H

#define SOMA_FREERTOS_CONFIG_INCLUDED 1

#include "soma/config.h"

#include "soma/assert.h"

#ifdef configCPU_CLOCK_HZ
#error configCPU_CLOCK_HZ is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configTICK_RATE_HZ
#error configTICK_RATE_HZ is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_PORT_OPTIMISED_TASK_SELECTION
#error configUSE_PORT_OPTIMISED_TASK_SELECTION is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configMINIMAL_STACK_SIZE
#error configMINIMAL_STACK_SIZE is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configTICK_TYPE_WIDTH_IN_BITS
#error configTICK_TYPE_WIDTH_IN_BITS is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_PREEMPTION
#error configUSE_PREEMPTION is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_TIME_SLICING
#error configUSE_TIME_SLICING is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configMAX_PRIORITIES
#error configMAX_PRIORITIES is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configMAX_TASK_NAME_LEN
#error configMAX_TASK_NAME_LEN is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configQUEUE_REGISTRY_SIZE
#error configQUEUE_REGISTRY_SIZE is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_IDLE_HOOK
#error configUSE_IDLE_HOOK is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_TICK_HOOK
#error configUSE_TICK_HOOK is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configCHECK_FOR_STACK_OVERFLOW
#error configCHECK_FOR_STACK_OVERFLOW is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configSUPPORT_STATIC_ALLOCATION
#error configSUPPORT_STATIC_ALLOCATION is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configSUPPORT_DYNAMIC_ALLOCATION
#error configSUPPORT_DYNAMIC_ALLOCATION is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_MUTEXES
#error configUSE_MUTEXES is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_RECURSIVE_MUTEXES
#error configUSE_RECURSIVE_MUTEXES is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_COUNTING_SEMAPHORES
#error configUSE_COUNTING_SEMAPHORES is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_TASK_NOTIFICATIONS
#error configUSE_TASK_NOTIFICATIONS is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configUSE_TRACE_FACILITY
#error configUSE_TRACE_FACILITY is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configRECORD_STACK_HIGH_ADDRESS
#error configRECORD_STACK_HIGH_ADDRESS is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configGENERATE_RUN_TIME_STATS
#error configGENERATE_RUN_TIME_STATS is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configRUN_TIME_COUNTER_TYPE
#error configRUN_TIME_COUNTER_TYPE is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_vTaskPrioritySet
#error INCLUDE_vTaskPrioritySet is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_uxTaskPriorityGet
#error INCLUDE_uxTaskPriorityGet is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_vTaskDelete
#error INCLUDE_vTaskDelete is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_vTaskSuspend
#error INCLUDE_vTaskSuspend is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskDelayUntil
#error INCLUDE_xTaskDelayUntil is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_vTaskDelay
#error INCLUDE_vTaskDelay is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskGetIdleTaskHandle
#error INCLUDE_xTaskGetIdleTaskHandle is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskAbortDelay
#error INCLUDE_xTaskAbortDelay is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xQueueGetMutexHolder
#error INCLUDE_xQueueGetMutexHolder is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xSemaphoreGetMutexHolder
#error INCLUDE_xSemaphoreGetMutexHolder is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskGetHandle
#error INCLUDE_xTaskGetHandle is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_uxTaskGetStackHighWaterMark
#error INCLUDE_uxTaskGetStackHighWaterMark is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_uxTaskGetStackHighWaterMark2
#error INCLUDE_uxTaskGetStackHighWaterMark2 is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_eTaskGetState
#error INCLUDE_eTaskGetState is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskResumeFromISR
#error INCLUDE_xTaskResumeFromISR is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTimerPendFunctionCall
#error INCLUDE_xTimerPendFunctionCall is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskGetSchedulerState
#error INCLUDE_xTaskGetSchedulerState is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef INCLUDE_xTaskGetCurrentTaskHandle
#error INCLUDE_xTaskGetCurrentTaskHandle is owned by SOMA and must not be defined in soma_config.h.
#endif
#ifdef configASSERT
#error configASSERT is owned by SOMA and must not be defined in soma_config.h.
#endif

// Platform-specific FreeRTOS port settings come directly from soma_config.h.

#if defined( configCPU_CLOCK_HZ ) || defined( configTICK_RATE_HZ )
    #error Configure clock and tick rate through SOMA_CONFIG_CPU_CLOCK_HZ and SOMA_CONFIG_TICK_RATE_HZ only.
#endif

// Translate the platform's settings into the names expected by FreeRTOS.
#define configCPU_CLOCK_HZ                       SOMA_CONFIG_CPU_CLOCK_HZ
#define configTICK_RATE_HZ                       SOMA_CONFIG_TICK_RATE_HZ

// FreeRTOS selects task-selection behavior from the chosen port.

#ifdef configMINIMAL_STACK_SIZE
    #error Configure the idle-task stack in bytes with SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES.
#endif
#define configMINIMAL_STACK_SIZE                 ( SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES / sizeof( StackType_t ) )

#if !defined( SOMA_CONFIG_TICK_TYPE_WIDTH_BITS )
    #error Define SOMA_CONFIG_TICK_TYPE_WIDTH_BITS in soma_config.h as 16, 32, or 64.
#endif

#if defined( configUSE_16_BIT_TICKS ) || defined( configTICK_TYPE_WIDTH_IN_BITS )
    #error Configure the tick width with SOMA_CONFIG_TICK_TYPE_WIDTH_BITS only.
#endif

#if SOMA_CONFIG_TICK_TYPE_WIDTH_BITS == 16
    #define configTICK_TYPE_WIDTH_IN_BITS         TICK_TYPE_WIDTH_16_BITS
#elif SOMA_CONFIG_TICK_TYPE_WIDTH_BITS == 32
    #define configTICK_TYPE_WIDTH_IN_BITS         TICK_TYPE_WIDTH_32_BITS
#elif SOMA_CONFIG_TICK_TYPE_WIDTH_BITS == 64
    #define configTICK_TYPE_WIDTH_IN_BITS         TICK_TYPE_WIDTH_64_BITS
#else
    #error SOMA_CONFIG_TICK_TYPE_WIDTH_BITS must be 16, 32, or 64.
#endif

// SOMA scheduling policy.
#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configMAX_PRIORITIES                     7
#define configMAX_TASK_NAME_LEN                  16u
#define configQUEUE_REGISTRY_SIZE                SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT
#ifdef configUSE_IDLE_HOOK
    #error FreeRTOS idle-hook configuration is owned by SOMA; use SOMA_CONFIG_IDLE_HOOK() for optional idle work.
#endif
#define configUSE_IDLE_HOOK                      1
#ifdef configUSE_TICK_HOOK
    #error FreeRTOS tick hooks are disabled by SOMA; configure the tick source in the platform port.
#endif
#define configUSE_TICK_HOOK                      0

#ifdef configCHECK_FOR_STACK_OVERFLOW
    #error SOMA selects stack-overflow checking based on NDEBUG; do not define configCHECK_FOR_STACK_OVERFLOW in soma_config.h.
#endif
#ifdef NDEBUG
    #define configCHECK_FOR_STACK_OVERFLOW        1
#else
    #define configCHECK_FOR_STACK_OVERFLOW        2
#endif

// Capabilities required by SOMA.
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_TRACE_FACILITY                 1
#define configRECORD_STACK_HIGH_ADDRESS          1
#define configGENERATE_RUN_TIME_STATS            1

#if defined( portGET_RUN_TIME_COUNTER_VALUE ) || defined( portALT_GET_RUN_TIME_COUNTER_VALUE )
    #error Provide the task-profiling counter through SOMA_CONFIG_GET_TASK_PROFILING_COUNTER(), not FreeRTOS port macros.
#endif

#if defined( SOMA_CONFIG_GET_TASK_PROFILING_COUNTER )
#define portGET_RUN_TIME_COUNTER_VALUE() SOMA_CONFIG_GET_TASK_PROFILING_COUNTER()
#else
// Keep runtime statistics available without requiring a platform timer.
// This default has one scheduler tick of resolution.
#define portGET_RUN_TIME_COUNTER_VALUE() xTaskGetTickCount()
#endif

// A platform-provided high-resolution counter must be initialized before the
// scheduler starts (normally in board_init()).
#ifdef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
    #error SOMA owns portCONFIGURE_TIMER_FOR_RUN_TIME_STATS; initialize a custom profiling counter before SOMA::run().
#endif
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() do { } while( 0 )

// The profiler stores raw counters as uint32_t and relies on unsigned
// 32-bit subtraction for wraparound-safe deltas. SOMA owns this setting.
#ifdef configRUN_TIME_COUNTER_TYPE
    #error SOMA fixes configRUN_TIME_COUNTER_TYPE to uint32_t; do not override it in soma_config.h.
#endif
#define configRUN_TIME_COUNTER_TYPE              uint32_t

// SOMA owns every INCLUDE_ switch supported by the bundled FreeRTOS kernel.
// Optional APIs retain their FreeRTOS defaults unless SOMA or its test fixture
// requires them; projects do not configure API visibility in soma_config.h.
#define INCLUDE_vTaskPrioritySet                 0
#define INCLUDE_uxTaskPriorityGet                0
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_xTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetIdleTaskHandle           1
#define INCLUDE_xTaskAbortDelay                  0
#define INCLUDE_xQueueGetMutexHolder             0
#define INCLUDE_xSemaphoreGetMutexHolder         0
#define INCLUDE_xTaskGetHandle                   0
#define INCLUDE_uxTaskGetStackHighWaterMark      0
#define INCLUDE_uxTaskGetStackHighWaterMark2     0
#define INCLUDE_eTaskGetState                    0
#define INCLUDE_xTaskResumeFromISR               1
#define INCLUDE_xTimerPendFunctionCall           0
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1

// Use the same C-compatible assertion path as SOMA clients.
#define configASSERT( expression ) SOMA_ASSERT( expression )

#endif // SOMA_FREERTOS_CONFIG_H
