// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/io/buffered_output_stream.h"
#include "soma/runtime/critical_section.h"

namespace SOMA
{
    Buffered_output_stream::Buffered_output_stream( String_view name,
                                                    I_output_stream& output_stream,
                                                    Ring_buffer<uint8_t>& buffer )
        : I_output_stream( name ),
          m_output_stream( output_stream ),
          m_ring_buffer( buffer )
    {
        m_output_stream.subscribe( m_observer_stream_ready );
    }

    Buffered_output_stream::~Buffered_output_stream()
    {
        m_output_stream.unsubscribe( m_observer_stream_ready );
    }

    bool Buffered_output_stream::start_writing( void )
    {
        bool rv_started = true;

        // start writing process from the local buffer
        while( m_ring_buffer.is_empty() == false )
        {
            auto data_buffer = m_ring_buffer.view_items();

            // setting bytes in transfer before calling write that can cause a context switch and calling the
            // output stream observer before this function actually returns
            m_bytes_in_transfer = data_buffer.size();

            Result_e write_result = m_output_stream.write( data_buffer );

            if( write_result == I_output_stream::Result_e::ready )
            {
                m_ring_buffer.displace( m_bytes_in_transfer );
            }
            else if( write_result == I_output_stream::Result_e::wait )
            // write in progress, will continue once the notification is received
            {
                break;
            }
            else if( write_result == I_output_stream::Result_e::unavailable )
            {
                // drop all buffered data
                m_ring_buffer.clear();
                m_upper_layer_buffer = Buffer_view<uint8_t>(); // invalidates the buffer

                rv_started = false;
            }
            else
            {
                ASSERT( false );
            }
        }

        return rv_started;
    }

    I_output_stream::Result_e Buffered_output_stream::write( Buffer_view<uint8_t> data )
    {
        Result_e rv_result = I_output_stream::Result_e::wait;

        ASSERT( data.size() > 0u );

        // the underlying stream is used in an async context which can also be an interrupt (so the mutex can not be used)
        Critical_section_guard critical_section;

        if( m_bytes_in_transfer == 0u )
        // the write process is in progress
        {
            critical_section.release();

            ASSERT( m_ring_buffer.is_empty() );

            size_t items_inserted = m_ring_buffer.insert( data );

            m_upper_layer_buffer = data.sub_buffer( items_inserted );

            if( m_upper_layer_buffer.size() == 0u )
            // all data buffered
            {
                rv_result = I_output_stream::Result_e::ready;
            }

            if( start_writing() == false )
            {
                rv_result = I_output_stream::Result_e::unavailable;
            }
        }
        else
        // write in progress
        {
            size_t items_inserted = m_ring_buffer.insert( data );

            m_upper_layer_buffer = data.sub_buffer( items_inserted );

            if( m_upper_layer_buffer.size() == 0u )
            // all data buffered
            {
                rv_result = I_output_stream::Result_e::ready;
            }

            critical_section.release();
        }

        return rv_result;
    }

    void Buffered_output_stream::async_callback_stream_ready( void )
    {
        Critical_section_guard critical_section;

        m_ring_buffer.displace( m_bytes_in_transfer );
        m_bytes_in_transfer = 0u;

        if( m_upper_layer_buffer.size() == 0u )
        // upper layer not waiting
        {
            start_writing(); // doesn't matter if it fails or not

            critical_section.release();
        }
        else
        // upper layer waiting
        {
            critical_section.release();

            size_t items_inserted = m_ring_buffer.insert( m_upper_layer_buffer );

            m_upper_layer_buffer = m_upper_layer_buffer.sub_buffer( items_inserted );

            bool stream_available = start_writing();

            if( ( stream_available == false ) ||
                ( m_upper_layer_buffer.size() == 0u ) )
            {
                // upper layer request processed
                notify();
            }
        }
    }
} // namespace SOMA
