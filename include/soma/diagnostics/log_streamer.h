// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_LOG_STREAMER_H_
#define SOMA_LOG_STREAMER_H_

#include <cstdint>

#include "soma/diagnostics/log.h"
#include "soma/io/output_stream_queue_consumer.h"
#include "soma/runtime/task_event.h"
#include "soma/runtime/task_startup_event.h"
#include "soma/runtime/task_timer.h"
#include "soma/time.h"

namespace SOMA
{
    class Event_task;

    ///
    /// @brief Active consumer that drains a Log into an output stream.
    class Log_streamer
    {
    public:
        explicit Log_streamer( Event_task& task,
                               Log& log,
                               uint32_t truncation_pause_ms,
                               I_output_stream& output_stream );

    private:
        const uint32_t mc_truncation_pause_ms;
        Log& m_log;

        Output_stream_queue_consumer<char> m_output_stream_consumer;
        Task_event<Log_streamer> m_event_log_full;
        Task_timer<Log_streamer> m_timer_pause;
        Task_startup_event<Log_streamer> m_event_startup;

        void on_startup( void );
        void on_log_full( void );
        void on_pause_timeout( void );
    };
} // namespace SOMA

#endif // LOG_STREAMER_H_
