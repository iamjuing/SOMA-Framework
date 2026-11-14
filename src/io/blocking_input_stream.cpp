// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/io/i_input_stream.h"
#include "soma/runtime/task.h"
#include "soma/io/blocking_input_stream.h"

namespace SOMA
{
    Blocking_input_stream::Blocking_input_stream( I_input_stream& input_stream )
        : m_input_stream( input_stream )
    {
        m_input_stream.subscribe( m_observer_stream_ready );
    }

    Blocking_input_stream::~Blocking_input_stream()
    {
        m_input_stream.unsubscribe( m_observer_stream_ready );
    }

    void Blocking_input_stream::read( Buffer<uint8_t>& out_buffer )
    {
        if( m_input_stream.read( out_buffer ) == I_input_stream::Result_e::wait )
        {
            m_synchronization_semaphore.wait();
        }
    }

    void Blocking_input_stream::read( Buffer<uint8_t>& out_buffer,
                                      uint32_t timeout_ms )
    {
        if( timeout_ms > 0u )
        {
            if( m_input_stream.read( out_buffer ) == I_input_stream::Result_e::wait )
            {
                if( m_synchronization_semaphore.wait( timeout_ms ) == false )
                {
                    m_input_stream.cancel_read();
                }
            }
        }
    }

    void Blocking_input_stream::async_callback_stream_ready( void )
    {
        m_synchronization_semaphore.signal();
    }
} // namespace SOMA
