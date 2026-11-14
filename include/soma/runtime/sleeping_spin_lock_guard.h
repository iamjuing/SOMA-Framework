// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SLEEPING_SPIN_LOCK_GUARD_H_
#define SOMA_SLEEPING_SPIN_LOCK_GUARD_H_

#include "soma/runtime/sleeping_spin_lock.h"

namespace SOMA
{
    // Acquires a Sleeping_spin_lock for this scope and releases it on exit.
    class Sleeping_spin_lock_guard
    {
    public:
        explicit Sleeping_spin_lock_guard( Sleeping_spin_lock& lock )
            : m_lock( lock )
        {
            m_lock.lock();
        }

        ~Sleeping_spin_lock_guard()
        {
            m_lock.unlock();
        }

        Sleeping_spin_lock_guard( const Sleeping_spin_lock_guard& ) = delete;
        Sleeping_spin_lock_guard& operator=( const Sleeping_spin_lock_guard& ) = delete;
        Sleeping_spin_lock_guard( Sleeping_spin_lock_guard&& ) = delete;
        Sleeping_spin_lock_guard& operator=( Sleeping_spin_lock_guard&& ) = delete;

    private:
        Sleeping_spin_lock& m_lock;
    };
}

#endif // SOMA_SLEEPING_SPIN_LOCK_GUARD_H_
