// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef WINDOWS_PLATFORM_PAL_H_
#define WINDOWS_PLATFORM_PAL_H_

#include "platform/windows/console_output_stream.h"
#include "platform/windows/console_input_stream.h"
#include "hardware_interface/i_led.h"


class Virtual_LED final: public I_LED
{
public:
    void set( bool is_on ) override
    {
        if( is_on )
        {
            ILOG << SOMA::Color::green << "LED ON" << SOMA::Color::standard;
        }
        else
        {
            ILOG << SOMA::Color::standard << "LED OFF";
        }
    }

private:
};

class PAL
{
public:
    PAL( void ) {};

    static constexpr size_t mc_task_memory_size_B = 2048u;
    static constexpr size_t mc_log_capacity_B = 4096u;

    SOMA::Task_memory<mc_task_memory_size_B> m_pal_task_memory;

    SOMA::Event_task m_pal_task { "pal", SOMA::Task::Priority_e::normal, m_pal_task_memory };

    Console_output_stream m_console { m_pal_task };
    Console_input_stream m_console_input { m_pal_task };
    Virtual_LED m_LED;
};

#endif // WINDOWS_PLATFORM_PAL_H_
