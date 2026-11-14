// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_CRITICAL_SECTION_H_
#define SOMA_CRITICAL_SECTION_H_

#include <cstdint>

namespace SOMA
{
    // Enters a critical section in the current task or interrupt context and
    // restores that context when the guard leaves scope.
    class Critical_section_guard
    {
    public:
        Critical_section_guard();
        ~Critical_section_guard();

        Critical_section_guard( const Critical_section_guard& ) = delete;
        Critical_section_guard& operator=( const Critical_section_guard& ) = delete;
        Critical_section_guard( Critical_section_guard&& ) = delete;
        Critical_section_guard& operator=( Critical_section_guard&& ) = delete;

        // Leaves the critical section early. Safe to call more than once.
        void release();

    private:
        bool m_from_isr;
        bool m_active = true;
        std::uintptr_t m_previous_state;
    };
}

#endif // SOMA_CRITICAL_SECTION_H_
