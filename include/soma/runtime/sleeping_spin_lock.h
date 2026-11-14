// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SLEEPING_SPIN_LOCK_H_
#define SOMA_SLEEPING_SPIN_LOCK_H_

#include <atomic>

namespace SOMA
{
    class Sleeping_spin_lock
    {
    public:
        void lock( void );
        void unlock();

    private:
        std::atomic_flag m_lock_flag = ATOMIC_FLAG_INIT;
    };
} // namespace SOMA

#endif // SOMA_SLEEPING_SPIN_LOCK_H_
