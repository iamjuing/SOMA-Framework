// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/io/i_output_stream.h"
#include "soma/runtime/task.h"
#include "soma/io/blocking_output_stream.h"
#include "soma/diagnostics/log.h"

namespace SOMA
{
    Blocking_output_stream::Blocking_output_stream( I_output_stream& output_stream )
    {
        subscribe( output_stream );
    }

    Blocking_output_stream::~Blocking_output_stream()
    {
        if( mp_output_stream != nullptr )
        {
            unsubscribe();
        }
    }

    bool Blocking_output_stream::write( Buffer_view<uint8_t> data )
    {
        bool rv_success = true;

        ASSERT( mp_output_stream != nullptr );

        if( data.size() > 0u )
        {
            const I_output_stream::Result_e write_result = mp_output_stream->write( data );

            if( write_result == I_output_stream::Result_e::wait )
            {
                m_synchronization_semaphore.wait();
            }
            else if( write_result == I_output_stream::Result_e::unavailable )
            {
                rv_success = false;
            }
            else
            {
                // intentionally empty
            }
        }

        return rv_success;
    }

    void Blocking_output_stream::async_callback_stream_ready( void )
    {
        m_synchronization_semaphore.signal();
    }

    void Blocking_output_stream::subscribe( I_output_stream& output_stream )
    {
        ASSERT( mp_output_stream == nullptr );
        mp_output_stream = &output_stream;
        mp_output_stream->subscribe( m_observer_handler_stream_ready );
    }
    void Blocking_output_stream::unsubscribe( void )
    {
        ASSERT( mp_output_stream != nullptr );
        mp_output_stream->unsubscribe( m_observer_handler_stream_ready );
        mp_output_stream = nullptr;
    }
} // namespace SOMA
