// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_MUTEX_H_
#define SOMA_MUTEX_H_

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"
#include "soma/runtime/task.h"
#include "soma/assert.h"

namespace SOMA
{
    class Mutex
    {
    public:
        Mutex( void );
        ~Mutex( void ) = default;

        bool is_locked( void ) const;
        bool try_lock( void ) const;

        void lock( void ) const;
        bool lock( uint32_t time_ms ) const;

        void unlock( void ) const;

    private:
        static constexpr uint32_t mc_interruption_time_ms = SOMA_CONFIG_SYNC_WAIT_CHUNK_MS;

        StaticSemaphore_t m_buffer;
        SemaphoreHandle_t m_handle;
    };
} // namespace SOMA

#endif // MUTEX_H_
