// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "platform/windows/console_input_stream.h"

#include "platform/windows/windows_host.h"
#include "soma/assert.h"
#include "soma/runtime/critical_section.h"

Console_input_stream::Console_input_stream( SOMA::Event_task& task )
    : SOMA::I_input_stream( "console" ),
      m_poll( task, *this, &Console_input_stream::on_poll )
{
    // intentionally empty
}

SOMA::I_input_stream::Result_e Console_input_stream::read( SOMA::Buffer<uint8_t>& out_buffer )
{
    ASSERT( out_buffer.space() > 0u );

    {
        SOMA::Critical_section_guard critical_section;

        if( try_read_console_input( out_buffer ) > 0u )
        {
            return Result_e::ready;
        }

        ASSERT( mp_pending_buffer == nullptr );
        mp_pending_buffer = &out_buffer;
    }

    m_poll.start_periodic( mc_poll_period_ms );

    bool is_read_pending = false;

    {
        SOMA::Critical_section_guard critical_section;
        is_read_pending = ( mp_pending_buffer == &out_buffer );
    }

    if( is_read_pending == false )
    {
        m_poll.cancel();
    }

    return Result_e::wait;
}

void Console_input_stream::cancel_read( void )
{
    {
        SOMA::Critical_section_guard critical_section;
        mp_pending_buffer = nullptr;
    }

    m_poll.cancel();
}

void Console_input_stream::on_poll( void )
{
    bool is_read_complete = false;

    {
        SOMA::Critical_section_guard critical_section;

        if( mp_pending_buffer != nullptr )
        {
            if( try_read_console_input( *mp_pending_buffer ) > 0u )
            {
                mp_pending_buffer = nullptr;
                is_read_complete = true;
            }
        }
    }

    if( is_read_complete )
    {
        m_poll.cancel();
        notify();
    }
}
