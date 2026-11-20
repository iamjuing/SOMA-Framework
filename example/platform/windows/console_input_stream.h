// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef WINDOWS_CONSOLE_INPUT_STREAM_H_
#define WINDOWS_CONSOLE_INPUT_STREAM_H_

#include "soma/io/i_input_stream.h"
#include "soma/runtime/event_task.h"
#include "soma/runtime/task_timer.h"

class Console_input_stream final: public SOMA::I_input_stream
{
public:
    explicit Console_input_stream( SOMA::Event_task& task );

    Result_e read( SOMA::Buffer<uint8_t>& out_buffer ) override;
    void cancel_read( void ) override;

private:
    static constexpr uint32_t mc_poll_period_ms = 10u;

    SOMA::Buffer<uint8_t>* mp_pending_buffer = nullptr;
    SOMA::Task_timer<Console_input_stream> m_poll;

    void on_poll( void );
};

#endif // WINDOWS_CONSOLE_INPUT_STREAM_H_
