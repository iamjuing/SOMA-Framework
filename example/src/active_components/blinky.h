// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef BLINKY_H_
#define BLINKY_H_

#include "soma.h"

#include "hardware_interface/i_led.h"

class Blinky
{
public:
    Blinky( SOMA::Event_task& task,
            I_LED& LED );

private:
    static constexpr uint32_t mc_blink_period_ms = 1000u;

    I_LED& m_LED;
    bool m_is_on = false;

    SOMA::Task_timer<Blinky> m_timer;
    SOMA::Task_startup_event<Blinky> m_startup;

    void on_startup( void );
    void on_timeout( void );
};

#endif // BLINKY_H_
