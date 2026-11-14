// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/runtime/critical_section.h"
#include "soma/runtime/task.h"

#include <cstdint>

namespace SOMA
{
    Critical_section_guard::Critical_section_guard()
        : m_from_isr( Task::in_ISR() ),
          m_previous_state( 0u )
    {
        if( m_from_isr )
        {
            m_previous_state = static_cast<std::uintptr_t>( taskENTER_CRITICAL_FROM_ISR() );
        }
        else
        {
            taskENTER_CRITICAL();
        }
    }

    Critical_section_guard::~Critical_section_guard()
    {
        release();
    }

    void Critical_section_guard::release()
    {
        if( m_active )
        {
            if( m_from_isr )
            {
                taskEXIT_CRITICAL_FROM_ISR( static_cast<UBaseType_t>( m_previous_state ) );
            }
            else
            {
                taskEXIT_CRITICAL();
            }

            m_active = false;
        }
    }
}
