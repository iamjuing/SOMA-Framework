// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/io/buffered_input_stream.h"
#include "soma/runtime/critical_section.h"

namespace SOMA
{
    namespace
    {
        constexpr size_t c_max_async_read_completion_size = 64u;
    }

    Buffered_input_stream::Buffered_input_stream( String_view name,
                                                  I_input_stream& input_stream,
                                                  Ring_buffer<uint8_t>& buffer )
        : I_input_stream( name ),
          m_input_stream( input_stream ),
          m_ring_buffer( buffer )
    {
        m_input_stream.subscribe( m_observer_stream_ready );

        // get all data from the stream and potentially start the reading process
        start_reading();
    }

    Buffered_input_stream::~Buffered_input_stream()
    {
        m_input_stream.unsubscribe( m_observer_stream_ready );
    }

    void Buffered_input_stream::start_reading( void )
    {
        while( m_ring_buffer.space() > 0u )
        {
            // setting before calling read that can cause a context switch
            m_transfer_buffer = m_ring_buffer.get_space();

            if( m_input_stream.read( m_transfer_buffer ) == I_input_stream::Result_e::wait )
            {
                break;
            }
            else
            {
                m_ring_buffer.place( m_transfer_buffer.size() );
                m_transfer_buffer = Buffer<uint8_t>(); // invalidates the buffer
            }
        }
    }

    I_input_stream::Result_e Buffered_input_stream::read( Buffer<uint8_t>& out_buffer )
    {
        Result_e rv_result = Result_e::wait;

        ASSERT( out_buffer.space() > 0u );
        ASSERT( mp_upper_layer_buffer == nullptr );

        // the underlying stream is used in an async context which can also be an interrupt (so the mutex can not be used)
        Critical_section_guard critical_section;

        if( m_transfer_buffer.capacity() == 0u )
        // the read process is not in progress (the ring buffer must be full)
        {
            critical_section.release();

            ASSERT( m_ring_buffer.remove( out_buffer ) > 0u );

            rv_result = Result_e::ready;

            // the ring buffer has now free space, start reading process
            start_reading();
        }
        else
        {
            if( m_ring_buffer.is_empty() )
            {
                mp_upper_layer_buffer = &out_buffer;

                critical_section.release();
            }
            else
            {
                critical_section.release();

                ASSERT( m_ring_buffer.remove( out_buffer ) > 0u );

                rv_result = Result_e::ready;
            }
        }

        return rv_result;
    }

    void Buffered_input_stream::cancel_read( void )
    {
        Critical_section_guard critical_section;

        mp_upper_layer_buffer = nullptr;
    }

    // runs in the context of the data source
    void Buffered_input_stream::async_callback_stream_ready( void )
    {
        bool notify_upper_layer = false;

        m_ring_buffer.place( m_transfer_buffer.size() );
        m_transfer_buffer = Buffer<uint8_t>(); // invalidates the buffer

        Critical_section_guard critical_section;

        if( mp_upper_layer_buffer != nullptr )
        {
            // if no data is available, the stream has become unavailable

            // only copy a fraction of the data to minimize the critical section
            m_ring_buffer.remove( *mp_upper_layer_buffer, c_max_async_read_completion_size );

            mp_upper_layer_buffer = nullptr; // invalidates the upper layer buffer

            notify_upper_layer = true;
        }

        critical_section.release();

        start_reading();

        if( notify_upper_layer )
        {
            notify();
        }
    }
} // namespace SOMA
