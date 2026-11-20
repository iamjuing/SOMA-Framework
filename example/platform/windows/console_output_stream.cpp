// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "platform/windows/console_output_stream.h"

#include "platform/windows/windows_host.h"

Console_output_stream::Console_output_stream( SOMA::Event_task& task )
    : SOMA::I_output_stream( "console" ),
      m_retry( task, *this, &Console_output_stream::on_retry )
{
    // intentionally empty
}

SOMA::I_output_stream::Result_e Console_output_stream::write( SOMA::Buffer_view<uint8_t> data )
{
    ASSERT( m_retry.is_started() == false );

    if( try_write_console( data ) )
    {
        return Result_e::ready;
    }

    // The stream contract keeps this view valid until completion. Retry on
    // the SOMA task so the native writer never needs to notify the scheduler.
    m_pending = data;
    m_retry.start_periodic( mc_retry_period_ms );

    return Result_e::wait;
}

void Console_output_stream::on_retry( void )
{
    if( try_write_console( m_pending ) )
    {
        m_pending = {};
        m_retry.cancel();
        notify();
    }
}
