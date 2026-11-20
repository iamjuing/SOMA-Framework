// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

// Host simulation settings; the CPU frequency is not a measured Windows clock.
#define SOMA_CONFIG_CPU_CLOCK_HZ                    100000000u
// A 10 ms tick avoids excessive host wakeups while retaining smooth blinking.
#define SOMA_CONFIG_TICK_RATE_HZ                    100u
#define SOMA_CONFIG_TICK_TYPE_WIDTH_BITS            32
#define SOMA_CONFIG_IDLE_TASK_STACK_SIZE_BYTES      2048u
#define SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH          150u

// Yield host CPU time when FreeRTOS has no application work, as in the test port.
// These declarations must also remain valid when included by FreeRTOS C code.
#ifdef __cplusplus
extern "C" {
#endif
void soma_firmware_idle_hook( void );
#ifdef __cplusplus
}

namespace SOMA
{
    class String_view;
}

extern "C++" [[noreturn]] void soma_firmware_panic( SOMA::String_view file_name,
                                                    int line,
                                                    SOMA::String_view message );
#endif

#define SOMA_CONFIG_IDLE_HOOK() soma_firmware_idle_hook()
#define SOMA_CONFIG_PANIC_HANDLER( file_name, line, message ) soma_firmware_panic( file_name, line, message )

#endif // BOARD_CONFIG_H_
