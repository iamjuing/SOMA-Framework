// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_OUTPUT_STREAM_QUEUE_CONSUMER_H_
#define SOMA_OUTPUT_STREAM_QUEUE_CONSUMER_H_

#include <algorithm>
#include <limits>

#include "soma/runtime/event_task.h"
#include "soma/runtime/task_event.h"
#include "soma/runtime/task_timer.h"
#include "soma/runtime/task_startup_event.h"
#include "soma/runtime/signal.h"
#include "soma/containers/buffer_view.h"
#include "soma/containers/i_single_consumer_queue.h"
#include "soma/io/i_output_stream.h"
#include "soma/time.h"

namespace SOMA
{

    template<typename T>
    class Output_stream_queue_consumer
    {
    public:
        enum class Unavailable_policy_e
        {
            discard,
            retain
        };

        ///
        /// @brief Drains a queue into a stream in the assigned task's context.
        ///        With retain, an unavailable stream leaves queued data intact and
        ///        pauses writes until resume() is called by the connection side channel.
        ///        Discard preserves the default behavior of dropping unavailable output.
        Output_stream_queue_consumer( Event_task& task,
                                      I_single_consumer_queue<T>& queue,
                                      I_output_stream& output_stream,
                                      uint32_t items_cumulation_time_ms,
                                      size_t max_chunk_size = std::numeric_limits<size_t>::max(),
                                      Unavailable_policy_e unavailable_policy = Unavailable_policy_e::discard )
            : m_queue( queue ),
              m_output_stream( output_stream ),
              mc_max_chunk_size( max_chunk_size ),
              mc_items_cumulation_time_ms( items_cumulation_time_ms ),
              mc_unavailable_policy( unavailable_policy ),
              m_event_startup( task, *this, &Output_stream_queue_consumer<T>::on_startup ),
              m_event_queue_items_available( task,
                                             m_queue.signal_items_available(),
                                             *this,
                                             &Output_stream_queue_consumer<T>::on_queue_items_available ),
              m_event_stream_ready( task, m_output_stream, *this, &Output_stream_queue_consumer<T>::on_stream_ready ),
              m_event_resume( task, m_signal_resume, *this, &Output_stream_queue_consumer<T>::on_resume ),
              m_cumulation_timer( task, *this, &Output_stream_queue_consumer<T>::on_cumulation_timeout )
        {
            static_assert( sizeof( T ) == 1, "Only byte-sized items are supported at the moment!" );
        }

        ///
        /// @brief Schedules a write attempt in the assigned task, bypassing batching delay.
        ///        May be called from another task or an ISR permitted to use FreeRTOS API.
        ///        Calls coalesce; an in-flight write remains pending until completion.
        ///        Call after the event task has started. Repeated unavailability pauses again.
        void resume( void )
        {
            m_signal_resume.emit();
        }

    private:
        I_single_consumer_queue<T>& m_queue;
        I_output_stream& m_output_stream;
        const size_t mc_max_chunk_size;
        const size_t mc_items_cumulation_time_ms;
        const Unavailable_policy_e mc_unavailable_policy;

        Signal m_signal_resume;

        Task_startup_event<Output_stream_queue_consumer<T>> m_event_startup;
        Task_event<Output_stream_queue_consumer<T>> m_event_queue_items_available;
        Task_event<Output_stream_queue_consumer<T>> m_event_stream_ready;
        Task_event<Output_stream_queue_consumer<T>> m_event_resume;
        Task_timer<Output_stream_queue_consumer<T>> m_cumulation_timer;

        size_t m_bytes_transmitting = 0u;
        bool m_is_paused = false;

        void write( void );
        void try_to_write( void );

        void on_startup( void );
        void on_queue_items_available( void );
        void on_stream_ready( void );
        void on_cumulation_timeout( void );
        void on_resume( void );
    };

    template<typename T>
    void Output_stream_queue_consumer<T>::write( void )
    {
        while( ( m_bytes_transmitting == 0u ) && ( m_is_paused == false ) )
        {
            auto write_buffer_candidate = m_queue.view_items();
            size_t items_to_write = std::min( write_buffer_candidate.size(), mc_max_chunk_size );

            auto data_buffer = Buffer_view( Array( reinterpret_cast<const uint8_t*>( write_buffer_candidate.array().storage() ),
                                                   items_to_write ) );

            if( data_buffer.size() > 0u )
            {
                I_output_stream::Result_e write_result = m_output_stream.write( data_buffer );

                if( write_result == I_output_stream::Result_e::ready )
                {
                    m_queue.displace( data_buffer.size() );
                }
                else if( write_result == I_output_stream::Result_e::unavailable )
                {
                    if( mc_unavailable_policy == Unavailable_policy_e::discard )
                    {
                        m_queue.displace( data_buffer.size() );
                    }
                    else
                    {
                        m_is_paused = true;
                        m_cumulation_timer.cancel();
                    }
                }
                else if( write_result == I_output_stream::Result_e::wait )
                // will get notification once the data is written
                {
                    m_bytes_transmitting = data_buffer.size();
                }
                else
                {
                    ASSERT( false );
                }
            }
            else
            // no items available in the queue
            {
                break;
            }
        }
    }

    template<typename T>
    void Output_stream_queue_consumer<T>::try_to_write( void )
    {
        if( m_is_paused )
        {
            return;
        }

        if( ( mc_items_cumulation_time_ms == 0u ) ||
            ( m_queue.size() >= mc_max_chunk_size ) )
        {
            write();
        }
        else if( ( m_cumulation_timer.is_started() == false ) &&
                 ( mc_items_cumulation_time_ms > 0u ) )
        {
            m_cumulation_timer.start( mc_items_cumulation_time_ms );
        }
        else
        {
            // intentionally empty
        }
    }

    template<typename T>
    void Output_stream_queue_consumer<T>::on_startup( void )
    {
        write();
    }

    template<typename T>
    void Output_stream_queue_consumer<T>::on_queue_items_available( void )
    {
        try_to_write();
    }

    template<typename T>
    void Output_stream_queue_consumer<T>::on_stream_ready( void )
    {
        m_queue.displace( m_bytes_transmitting );

        m_bytes_transmitting = 0u;

        try_to_write();
    }

    template<typename T>
    void Output_stream_queue_consumer<T>::on_cumulation_timeout( void )
    {
        write();
    }

    template<typename T>
    void Output_stream_queue_consumer<T>::on_resume( void )
    {
        m_is_paused = false;
        m_cumulation_timer.cancel();
        write();
    }
} // namespace SOMA

#endif // OUTPUT_STREAM_QUEUE_CONSUMER_H_
