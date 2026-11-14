// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

// SOMA configuration template.
// Copy this file into your firmware project's config directory as soma_config.h
// and enable the required library and port settings.

// Keep it valid C: FreeRTOS and SOMA C sources include this file.

#ifndef SOMA_CONFIG_H_
#define SOMA_CONFIG_H_

// -----------------------------------------------------------------------------
// Board parameters
// -----------------------------------------------------------------------------

// #define SOMA_CONFIG_CPU_CLOCK_HZ                       100000000u
// #define SOMA_CONFIG_TICK_RATE_HZ                       1000u
// #define SOMA_CONFIG_TICK_TYPE_WIDTH_BITS               32
// #define SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES         2048u

// -----------------------------------------------------------------------------
// FreeRTOS port related configuration
// -----------------------------------------------------------------------------

// #define configPRIO_BITS                                  4
// #define configKERNEL_INTERRUPT_PRIORITY                  (15u << (8u - configPRIO_BITS))
// #define configMAX_SYSCALL_INTERRUPT_PRIORITY             (5u << (8u - configPRIO_BITS))
// #define vPortSVCHandler                                  SVC_Handler
// #define xPortPendSVHandler                               PendSV_Handler
// #define xPortSysTickHandler                              SysTick_Handler

// -----------------------------------------------------------------------------
// Runtime behavior
// -----------------------------------------------------------------------------

// Optional. Maximum time a SOMA mutex or semaphore wait blocks between
// watchdog-visible retries. Default: 100 ms. Must convert to at least one
// scheduler tick and remain below the kernel's maximum finite delay.
// #define SOMA_CONFIG_SYNC_WAIT_CHUNK_MS              100u

// -----------------------------------------------------------------------------
// Memory and allocation
// -----------------------------------------------------------------------------

// Optional. Route global new/delete (including aligned forms) through SOMA's
// configured heap. Initialize the heap before any C++ dynamic allocation.
// #define SOMA_CONFIG_OVERRIDE_GLOBAL_NEW_DELETE

// -----------------------------------------------------------------------------
// Variables
// -----------------------------------------------------------------------------

// Optional. Enable variable names, named constructors, and name-based lookup.
// B_variable remains enumerable whether this is enabled or not.
// #define SOMA_CONFIG_ENABLE_NAMED_VARIABLES

// -----------------------------------------------------------------------------
// Diagnostics
// -----------------------------------------------------------------------------

// Required. Maximum number of characters stored in one log message.
// #define SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH          150u

// Optional. Define to compile DLOG in every translation unit that includes
// SOMA logging headers. It can instead be defined before those headers in only
// the translation units that need debug logging.
// #define SOMA_CONFIG_ENABLE_DLOG

// Optional. Override the default scheduler-tick profiling counter with a
// higher-resolution, monotonically increasing 32-bit counter. Initialize and
// start the counter in board_init(), before SOMA::run(); keep it running and
// ensure its wrap period exceeds the longest profiling window.
// #define SOMA_CONFIG_GET_TASK_PROFILING_COUNTER()    board_task_counter_read()

// Optional. Maximum number of simultaneously live regular FreeRTOS queues
// tracked by Memory_profiler. Zero disables FreeRTOS queue profiling. Default: 0.
// #define SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT        32u

// -----------------------------------------------------------------------------
// Platform hooks and failures
// -----------------------------------------------------------------------------

// Optional. Short, non-blocking work to run whenever the idle task executes.
// The macro body must be valid in both C and C++.
// #define SOMA_CONFIG_IDLE_HOOK()                     board_idle_hook()

// Required. The platform owns interrupt masking, diagnostics and halt/reset.
// Arguments are SOMA::String_view, int and SOMA::String_view. Diagnostic views
// are empty and the line is zero in optimized builds. Do not use RTOS APIs,
// normal logging or allocation; the hook can run before startup or in an ISR.
// Forward-declare String_view and the hook under __cplusplus rather than
// including C++ headers here. Give the hook explicit C++ or C linkage because
// FreeRTOS can include this configuration inside extern "C". SOMA spins forever
// if the hook returns.
// #define SOMA_CONFIG_PANIC_HANDLER(file_name, line, message) board_panic(file_name, line, message)

#endif // SOMA_CONFIG_H_
