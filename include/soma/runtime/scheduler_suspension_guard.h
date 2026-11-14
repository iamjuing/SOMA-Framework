// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SCHEDULER_SUSPENSION_GUARD_H_
#define SOMA_SCHEDULER_SUSPENSION_GUARD_H_

namespace SOMA
{
    // Suspends task scheduling until this guard leaves scope. Task context only.
    class Scheduler_suspension_guard
    {
    public:
        Scheduler_suspension_guard();
        ~Scheduler_suspension_guard();

        Scheduler_suspension_guard( const Scheduler_suspension_guard& ) = delete;
        Scheduler_suspension_guard& operator=( const Scheduler_suspension_guard& ) = delete;
        Scheduler_suspension_guard( Scheduler_suspension_guard&& ) = delete;
        Scheduler_suspension_guard& operator=( Scheduler_suspension_guard&& ) = delete;

        // Resumes the scheduler early. Safe to call more than once.
        void release();

    private:
        bool m_active = true;
    };
}

#endif // SOMA_SCHEDULER_SUSPENSION_GUARD_H_
