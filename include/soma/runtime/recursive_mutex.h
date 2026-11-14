// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_RECURSIVE_MUTEX_H_
#define SOMA_RECURSIVE_MUTEX_H_

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"
#include "soma/assert.h"
#include "soma/runtime/task.h"

namespace SOMA
{
    class Recursive_mutex
    {
    public:
        Recursive_mutex( void );
        ~Recursive_mutex( void ) = default;

        bool try_lock( void );

        void lock( void );
        bool lock( uint32_t time_ms );

        void unlock( void );

    private:
        static constexpr uint32_t mc_interruption_time_ms = SOMA_CONFIG_SYNC_WAIT_CHUNK_MS;

        StaticSemaphore_t m_buffer;
        SemaphoreHandle_t m_handle;
    };
} // namespace SOMA

#endif // RECURSIVE_MUTEX_H_
