// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef WINDOWS_CONSOLE_OUTPUT_STREAM_H_
#define WINDOWS_CONSOLE_OUTPUT_STREAM_H_

#include "soma.h"

class Console_output_stream: public SOMA::I_output_stream
{
public:
    explicit Console_output_stream( SOMA::Event_task& task );

    Result_e write( SOMA::Buffer_view<uint8_t> data ) override;

private:
    static constexpr uint32_t mc_retry_period_ms = 10u;

    SOMA::Buffer_view<uint8_t> m_pending;
    SOMA::Task_timer<Console_output_stream> m_retry;

    void on_retry( void );
};

#endif // WINDOWS_CONSOLE_OUTPUT_STREAM_H_
