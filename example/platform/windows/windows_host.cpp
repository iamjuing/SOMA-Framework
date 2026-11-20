// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <windows.h>

#include <algorithm>

#include "FreeRTOS/include/FreeRTOS.h"
#include "platform/windows/windows_host.h"
#include "soma/containers/ring_buffer.h"
#include "soma/text/string_view.h"

namespace
{
    constexpr size_t c_console_capacity_B = 8192u;
    constexpr size_t c_console_chunk_size_B = 256u;
    constexpr DWORD c_console_poll_period_ms = 10u;

    SOMA::Ring_buffer<uint8_t, c_console_capacity_B> g_console_buffer;
    SOMA::Ring_buffer<uint8_t, c_console_capacity_B> g_console_input_buffer;
    HANDLE g_console_handle = INVALID_HANDLE_VALUE;
    HANDLE g_console_input_handle = INVALID_HANDLE_VALUE;
    DWORD g_scheduler_thread_id = 0u;

    bool write_all( HANDLE handle,
                    SOMA::Buffer_view<uint8_t> data )
    {
        size_t offset = 0u;

        while( offset < data.size() )
        {
            DWORD written = 0u;
            const bool is_written = WriteFile( handle,
                                               data.array().storage() + offset,
                                               static_cast<DWORD>( data.size() - offset ),
                                               &written,
                                               nullptr ) != FALSE;

            if( ( is_written == false ) || ( written == 0u ) )
            {
                return false;
            }

            offset += written;
        }

        return true;
    }

    DWORD WINAPI console_worker( void* )
    {
        SOMA::Buffer<uint8_t, c_console_chunk_size_B> chunk;

        for(;;)
        {
            chunk.clear();
            g_console_buffer.remove( chunk );

            if( chunk.size() > 0u )
            {
                if( write_all( g_console_handle, chunk.view_items() ) == false )
                {
                    ExitProcess( EXIT_FAILURE );
                }
            }
            else
            {
                Sleep( c_console_poll_period_ms );
            }
        }
    }

    DWORD WINAPI console_input_worker( void* )
    {
        for(;;)
        {
            SOMA::Buffer<uint8_t> input_buffer = g_console_input_buffer.get_space();

            if( input_buffer.space() == 0u )
            {
                Sleep( c_console_poll_period_ms );
                continue;
            }

            const auto input_storage = input_buffer.get_space();
            const DWORD read_capacity = static_cast<DWORD>( std::min( input_buffer.space(),
                                                                      c_console_chunk_size_B ) );
            DWORD bytes_read = 0u;

            if( ( ReadFile( g_console_input_handle,
                            input_storage.storage(),
                            read_capacity,
                            &bytes_read,
                            nullptr ) != FALSE ) && ( bytes_read > 0u ) )
            {
                g_console_input_buffer.place( bytes_read );
            }
            else
            {
                Sleep( c_console_poll_period_ms );
            }
        }
    }
}

    bool initialize_windows_host( void )
    {
        g_scheduler_thread_id = GetCurrentThreadId();
        g_console_handle = GetStdHandle( STD_OUTPUT_HANDLE );

        if( ( g_console_handle == nullptr ) || ( g_console_handle == INVALID_HANDLE_VALUE ) )
        {
            return false;
        }

        DWORD console_mode = 0u;

        if( GetConsoleMode( g_console_handle, &console_mode ) != FALSE )
        {
            // Redirected output is also supported; only terminals have a mode.
            SetConsoleMode( g_console_handle, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
        }

        HANDLE thread = CreateThread( nullptr, 0u, console_worker, nullptr, 0u, nullptr );

        if( thread == nullptr )
        {
            return false;
        }

        // The process owns this worker's lifetime; no thread handle is needed.
        CloseHandle( thread );

        g_console_input_handle = GetStdHandle( STD_INPUT_HANDLE );

        if( ( g_console_input_handle != nullptr ) && ( g_console_input_handle != INVALID_HANDLE_VALUE ) )
        {
            HANDLE input_thread = CreateThread( nullptr, 0u, console_input_worker, nullptr, 0u, nullptr );

            if( input_thread == nullptr )
            {
                return false;
            }

            CloseHandle( input_thread );
        }

        return true;
    }

    bool try_write_console( SOMA::Buffer_view<uint8_t> data )
    {
        if( data.size() > g_console_buffer.space() )
        {
            return false;
        }

        const size_t inserted = g_console_buffer.insert( data );

        ASSERT( inserted == data.size() );

        return true;
    }

    size_t try_read_console_input( SOMA::Buffer<uint8_t>& out_buffer )
    {
        return g_console_input_buffer.remove( out_buffer );
    }

// The Windows port exports its interrupt-state flag but not SOMA's query hook.
// Also check thread identity: native console work is never an interrupt context.
BaseType_t xPortIsInsideInterrupt( void )
{
    return ( ( GetCurrentThreadId() == g_scheduler_thread_id ) && ( xInsideInterrupt != pdFALSE ) ) ? pdTRUE : pdFALSE;
}

extern "C" void soma_firmware_idle_hook( void )
{
    // The Windows port runs its task threads at elevated host priority.
    Sleep( 1u );
}

[[noreturn]] void soma_firmware_panic( SOMA::String_view file_name,
                                       int line,
                                       SOMA::String_view message )
{
    // Emergency output bypasses logging, allocation, CRT locks, and the scheduler.
    const HANDLE error_handle = GetStdHandle( STD_ERROR_HANDLE );
    write_all( error_handle, SOMA::String_view( "SOMA firmware panic: " ) );
    write_all( error_handle, file_name );

    if( line > 0 )
    {
        constexpr size_t c_line_number_capacity = 10u;
        uint8_t digits[c_line_number_capacity] {};
        size_t start = c_line_number_capacity;
        unsigned int remaining = static_cast<unsigned int>( line );

        while( remaining > 0u )
        {
            --start;
            digits[start] = static_cast<uint8_t>( '0' + ( remaining % 10u ) );
            remaining /= 10u;
        }

        write_all( error_handle, SOMA::String_view( ":" ) );
        write_all( error_handle,
                   SOMA::Buffer_view<uint8_t>( SOMA::Array<const uint8_t>( digits + start,
                                                                           c_line_number_capacity - start ) ) );
    }

    write_all( error_handle, SOMA::String_view( ": " ) );
    write_all( error_handle, message );
    write_all( error_handle, SOMA::String_view( "\r\n" ) );

    if( IsDebuggerPresent() != FALSE )
    {
        DebugBreak();
    }

    ExitProcess( EXIT_FAILURE );
}
