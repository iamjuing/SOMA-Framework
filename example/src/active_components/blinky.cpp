// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "src/active_components/blinky.h"

#include "soma/diagnostics/log.h"

Blinky::Blinky( SOMA::Event_task& task,
                I_LED& LED )
    : m_LED( LED ),
      m_timer( task, *this, &Blinky::on_timeout ),
      m_startup( task, *this, &Blinky::on_startup )
{
    // intentionally empty
}

void Blinky::on_startup( void )
{
    m_LED.set( m_is_on );

    m_timer.start_periodic( mc_blink_period_ms );
}

void Blinky::on_timeout( void )
{
    m_is_on = !m_is_on;

    m_LED.set( m_is_on );
}
