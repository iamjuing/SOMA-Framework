// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_INPUT_STREAM_QUEUE_PRODUCER_H_
#define SOMA_INPUT_STREAM_QUEUE_PRODUCER_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "soma/containers/buffer.h"
#include "soma/io/i_input_stream.h"
#include "soma/containers/i_single_producer_queue.h"
#include "soma/runtime/task_event.h"
#include "soma/runtime/task_startup_event.h"

namespace SOMA
{
    template<typename T>
    class Input_stream_queue_producer
    {
    public:
        Input_stream_queue_producer( Event_task& task,
                                     I_single_producer_queue<T>& queue,
                                     I_input_stream& input_stream,
                                     size_t max_chunk_size = std::numeric_limits<size_t>::max() )
            : m_queue( queue ),
              m_input_stream( input_stream ),
              mc_max_chunk_size( max_chunk_size ),
              m_event_startup( task, *this, &Input_stream_queue_producer<T>::on_startup ),
              m_event_queue_space_available( task,
                                             m_queue.signal_space_available(),
                                             *this,
                                             &Input_stream_queue_producer<T>::on_queue_space_available ),
              m_event_stream_ready( task, m_input_stream, *this, &Input_stream_queue_producer<T>::on_stream_ready )
        {
            static_assert( sizeof( T ) == 1, "Only byte-sized items are supported at the moment!" );
        }

    private:
        I_single_producer_queue<T>& m_queue;
        I_input_stream& m_input_stream;
        const size_t mc_max_chunk_size;

        Task_startup_event<Input_stream_queue_producer<T>> m_event_startup;
        Task_event<Input_stream_queue_producer<T>> m_event_queue_space_available;
        Task_event<Input_stream_queue_producer<T>> m_event_stream_ready;

        Buffer<uint8_t> m_read_buffer;

        bool m_read_in_progress = false;

        void read( void )
        {
            if( m_read_in_progress == false )
            {
                do
                {
                    auto read_buffer_candidate = m_queue.get_space();
                    size_t items_to_read = std::min( read_buffer_candidate.space(), mc_max_chunk_size );

                    m_read_buffer = Buffer( reinterpret_cast<uint8_t*>( read_buffer_candidate.storage() ),
                                            items_to_read );

                    if( items_to_read > 0u )
                    {
                        size_t bytes_read = m_input_stream.read( m_read_buffer );

                        if( bytes_read > 0u )
                        {
                            m_queue.place( bytes_read );
                        }
                        else
                        // will get notification once the data is read
                        {
                            m_read_in_progress = true;
                        }
                    }
                    else
                    // queue full, wait for space to become available
                    {
                        break;
                    }
                } while( m_read_in_progress == false );
            }
        }

        void on_startup( void )
        {
            read();
        }

        // called from a task context not to delay the queue consumer generating the signal
        void on_queue_space_available( void )
        {
            read();
        }

        // called from the input stream context not to introduce delays between reads
        // NOTE: Introducing a delay could cause the input stream to lose data so,
        //       providing the queue buffer (by calling read) immediately is critical.
        void on_stream_ready( void )
        {
            m_queue.place( m_read_buffer.size() );

            m_read_in_progress = false;

            read();
        }
    };
} // namespace SOMA

#endif // INPUT_STREAM_QUEUE_PRODUCER_H_
