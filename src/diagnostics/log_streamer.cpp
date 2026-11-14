// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/diagnostics/log_streamer.h"

#include "soma/runtime/event_task.h"

namespace SOMA
{
    namespace
    {
        constexpr size_t c_max_lines_per_write = 50u;
    }

    Log_streamer::Log_streamer( Event_task& task,
                                Log& log,
                                uint32_t truncation_pause_ms,
                                I_output_stream& output_stream )
        : mc_truncation_pause_ms( truncation_pause_ms ),
          m_log( log ),
          m_output_stream_consumer( task,
                                    m_log.consumer_queue(),
                                    output_stream,
                                    200u,
                                    c_max_lines_per_write * Log::line_buffer_size() ),
          m_event_log_full( task, m_log.log_paused_signal(), *this, &Log_streamer::on_log_full ),
          m_timer_pause( task, *this, &Log_streamer::on_pause_timeout ),
          m_event_startup( task, *this, &Log_streamer::on_startup )
    {
        // intentionally empty
    }

    void Log_streamer::on_startup( void )
    {
        if( m_log.is_paused() )
        {
            on_log_full();
        }
    }

    void Log_streamer::on_log_full( void )
    {
        // start polling for free space (flushing is managed by the output stream consumer in the background)
        m_timer_pause.start( mc_truncation_pause_ms );
    }

    void Log_streamer::on_pause_timeout( void )
    {
        // continue polling for free space
        if( m_log.try_resume() == false )
        {
            m_timer_pause.start( mc_truncation_pause_ms );
        }
    }
} // namespace SOMA
