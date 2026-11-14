// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TASK_MEMORY_H_
#define SOMA_TASK_MEMORY_H_

#include <cstdint>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/array.h"

namespace SOMA
{

    class B_task_memory
    {
    public:
        explicit B_task_memory( Array<StackType_t> array )
            : m_stack( array )
        {
            // intentionally empty
        }

        Array<StackType_t> stack() const
        {
            return m_stack;
        }

        StaticTask_t& TCB()
        {
            return m_TCB;
        }

    private:
        Array<StackType_t> m_stack;
        StaticTask_t m_TCB;
    };

    template<size_t N>
    class Task_memory: public B_task_memory
    {
    public:
        Task_memory( void );

    private:
        StackType_t m_stack[N / sizeof( StackType_t )];
    };

    template<size_t N>
    Task_memory<N>::Task_memory( void )
        : B_task_memory( Array<StackType_t>( m_stack, N / sizeof( StackType_t ) ) )
    {
        // intentionally empty
    }
} // namespace SOMA

#endif // TASK_MEMORY_H_
