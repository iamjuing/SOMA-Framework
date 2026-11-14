// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <chrono>

#include "test_common.h"

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/runtime/task.h"

using namespace SOMA;

void busy_loop( size_t time_ms )
{
    auto end = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds( time_ms );

    while( std::chrono::high_resolution_clock::now() < end );
}

bool wait_until_true( size_t timeout_ms,
                      std::function<bool()> condition )
{
    bool rv_timed_out = true;
    const uint32_t c_sleep_period_ms = 20;

    for( size_t i = 0; i < ( timeout_ms / c_sleep_period_ms ); ++i )
    {
        if( condition() )
        {
            rv_timed_out = false;
            break;
        }

        Task::sleep( c_sleep_period_ms );
    }

    return rv_timed_out == false;
}
