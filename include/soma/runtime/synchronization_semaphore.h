// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SYNCHRONIZATION_SEMAPHORE_H_
#define SOMA_SYNCHRONIZATION_SEMAPHORE_H_

#include "soma/runtime/binary_semaphore.h"

namespace SOMA
{

    ///
    /// @brief A single pending signal for synchronizing tasks with callbacks.
    ///
    /// Each successful wait consumes the signal. Repeated signals before a wait
    /// coalesce into one. A new instance has no pending signal.
    class Synchronization_semaphore
    {
    public:
        bool try_wait( void ) const;
        void wait( void ) const;
        bool wait( uint32_t timeout_ms ) const;
        void signal( void ) const;

    private:
        Binary_semaphore m_semaphore { Binary_semaphore::Initial_state_e::busy };
    };

    inline bool Synchronization_semaphore::try_wait( void ) const
    {
        return m_semaphore.try_take();
    }

    inline void Synchronization_semaphore::wait( void ) const
    {
        m_semaphore.take();
    }

    inline bool Synchronization_semaphore::wait( uint32_t timeout_ms ) const
    {
        return m_semaphore.take( timeout_ms );
    }

    inline void Synchronization_semaphore::signal( void ) const
    {
        m_semaphore.give();
    }
} // namespace SOMA

#endif // SYNCHRONIZATION_SEMAPHORE_H_
