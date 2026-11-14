// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#pragma once

#include "FreeRTOS/include/FreeRTOS.h"

#include <atomic>
#include <cstdint>
#include <limits>

namespace SOMA
{
    namespace Internal
    {
        constexpr uint32_t c_no_notification_bits = 0u;
        constexpr uint32_t c_all_notification_bits = std::numeric_limits<uint32_t>::max();

        inline std::atomic_bool g_application_launched { false };

        template<typename T>
        constexpr T max_interval( void )
        {
            return std::numeric_limits<T>::max() >> 1u;
        }

        TickType_t milliseconds_to_ticks( uint32_t time_ms );
        uint32_t ticks_to_milliseconds( TickType_t ticks );
        TickType_t tick_count( void );
        TickType_t ticks_remaining( TickType_t deadline );

        inline bool ticks_before( TickType_t lhs,
                                  TickType_t rhs )
        {
            const TickType_t tick_difference = lhs - rhs;
            const bool rv_before = tick_difference > max_interval<TickType_t>();

            return rv_before;
        }

        bool wait_task_notification_ticks( uint32_t& out_notification_value,
                                           uint32_t clear_on_entry_mask,
                                           uint32_t clear_on_exit_mask,
                                           TickType_t timeout_ticks );
    }
}
