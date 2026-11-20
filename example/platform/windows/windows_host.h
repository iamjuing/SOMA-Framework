// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef PLATFORM_WINDOWS_HOST_H_
#define PLATFORM_WINDOWS_HOST_H_

#include <cstddef>
#include <cstdint>

#include "soma/containers/buffer.h"
#include "soma/containers/buffer_view.h"

// Call once before starting FreeRTOS. The native console workers live until
// process exit and never call FreeRTOS or publish SOMA notifications.
bool initialize_windows_host( void );

// Single producer: the application's console stream. Copies the entire view
// or returns false without consuming it; never waits for Windows console I/O.
bool try_write_console( SOMA::Buffer_view<uint8_t> data );

// Non-blockingly reads input bytes captured by the native console worker.
size_t try_read_console_input( SOMA::Buffer<uint8_t>& out_buffer );

#endif // PLATFORM_WINDOWS_HOST_H_
