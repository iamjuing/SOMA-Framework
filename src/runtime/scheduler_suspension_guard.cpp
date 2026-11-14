// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/runtime/scheduler_suspension_guard.h"

namespace SOMA
{
    Scheduler_suspension_guard::Scheduler_suspension_guard()
    {
        vTaskSuspendAll();
    }

    Scheduler_suspension_guard::~Scheduler_suspension_guard()
    {
        release();
    }

    void Scheduler_suspension_guard::release()
    {
        if( m_active )
        {
            (void)xTaskResumeAll();
            m_active = false;
        }
    }
}
