// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

// Unit-test configuration; categories and available settings mirror the
// dev/soma_config.example.h template.

#ifndef SOMA_CONFIG_H_
#define SOMA_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Board parameters
// -----------------------------------------------------------------------------
#define SOMA_CONFIG_CPU_CLOCK_HZ                    100'000'000u
#define SOMA_CONFIG_TICK_RATE_HZ                    1'000u // SOMA::Time::on_tick() shall be called at this rate
#define SOMA_CONFIG_TICK_TYPE_WIDTH_BITS            32
#define SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES      2'048u

// -----------------------------------------------------------------------------
// FreeRTOS port related configuration
// -----------------------------------------------------------------------------
// none

// -----------------------------------------------------------------------------
// Runtime behavior
// -----------------------------------------------------------------------------
// Maximum time a SOMA mutex or semaphore wait blocks between watchdog-visible
// retries. Default is 100 ms; it must convert to at least one tick and stay
// below the kernel's maximum finite delay.
#define SOMA_CONFIG_SYNC_WAIT_CHUNK_MS              100u

// -----------------------------------------------------------------------------
// Memory and allocation
// -----------------------------------------------------------------------------
// Route global new/delete, including aligned forms, through SOMA's configured
// heap. Heap initialization must happen before any C++ dynamic allocation.
//#define SOMA_CONFIG_OVERRIDE_GLOBAL_NEW_DELETE

// -----------------------------------------------------------------------------
// Variables
// -----------------------------------------------------------------------------
// Named variable storage and lookup are disabled in the generic unit-test
// fixture. Named constructors remain available and ignore the supplied name.
//#define SOMA_CONFIG_ENABLE_NAMED_VARIABLES

// -----------------------------------------------------------------------------
// Diagnostics
// -----------------------------------------------------------------------------
// Maximum number of characters stored in one log message.
#define SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH          150u

// Enable DLOG in all translation units that include SOMA logging headers, or
// define it before those headers in only the files that need debug logging.
//#define SOMA_CONFIG_ENABLE_DLOG

// The default scheduler-tick counter is enough for coarse profiling in tests.
// For finer resolution, provide a monotonic 32-bit hardware counter, initialize
// it before SOMA::run(), and ensure its wrap period exceeds the longest profile
// window.
//#define SOMA_CONFIG_GET_TASK_PROFILING_COUNTER()    board_task_counter_read()

#define SOMA_CONFIG_MAX_FREERTOS_QUEUE_COUNT        32u

// -----------------------------------------------------------------------------
// Platform hooks and failures
// -----------------------------------------------------------------------------
// Optional short, non-blocking work performed whenever the idle task runs.
#define SOMA_CONFIG_IDLE_HOOK()                     Sleep( 1 )

// Host panic hook prints diagnostics and exits without using the scheduler.
#ifdef __cplusplus
namespace SOMA
{
    class String_view;
}

extern "C++" [[noreturn]] void soma_test_panic( SOMA::String_view file_name,
                                                int line,
                                                SOMA::String_view message );
#endif
#define SOMA_CONFIG_PANIC_HANDLER( file_name, line, message ) soma_test_panic( file_name, line, message )

#endif // SOMA_CONFIG_H_
