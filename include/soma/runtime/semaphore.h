// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SEMAPHORE_H_
#define SOMA_SEMAPHORE_H_

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"
#include "soma/runtime/task.h"
#include "soma/assert.h"

namespace SOMA
{
    class Semaphore
    {
    public:
        Semaphore( uint32_t max_count,
                   uint32_t initial_count = 0 );
        ~Semaphore( void ) = default;

        bool try_take( void ) const;
        void take( void ) const;
        bool take( uint32_t time_ms ) const;
        void give( void ) const;

    private:
        static constexpr uint32_t mc_interruption_time_ms = SOMA_CONFIG_SYNC_WAIT_CHUNK_MS;

        StaticSemaphore_t m_buffer;
        SemaphoreHandle_t m_handle;
    };
} // namespace SOMA

#endif // SEMAPHORE_H_
