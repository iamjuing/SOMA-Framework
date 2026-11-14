// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_RUN_H_
#define SOMA_RUN_H_

#include "soma/assert.h"
#include "soma/runtime/task_memory.h"
#include "soma/runtime/heap.h"

namespace SOMA
{
    namespace Internal
    {
        [[noreturn]] void run_after_heap_initialization( void ( *startup_function ) ( void* ),
                                                         B_task_memory& startup_task_memory );
    }

    ///
    /// @brief Creates the application startup task and starts the FreeRTOS scheduler.
    ///
    /// @tparam N Number of heap regions; zero disables heap use.
    /// @param startup_function Function executed by the startup task.
    /// @param startup_task_memory Static storage used by the startup task.
    /// @param heap_regions Views of writable memory regions; the view size must equal N.
    ///
    /// Application startup sequence:
    ///
    /// HAL initialization, including hardware needed by heap memory (client startup code)
    ///          |
    ///          v
    /// run<N>( startup_function, startup_task_memory, heap_regions ) (client startup code)
    ///          |
    ///          v
    /// heap initialization (SOMA)
    ///          |
    ///          v
    /// creation of the startup task (SOMA)
    ///          |
    ///          v
    /// scheduler start (SOMA)
    ///          |
    ///          v
    /// startup_function( nullptr ) (client startup code, running in the startup task)
    ///          |
    ///          v
    /// application identification and selection (optional, client startup code)
    ///          |
    ///          v
    /// Application_factory::create<T>() (client startup code)
    ///          |
    ///          v
    /// PAL construction (C++ base-class construction order)
    ///          |
    ///          v
    /// B_application members, derived application members, and derived constructor body
    ///          |    (C++ construction order)
    ///          v
    /// application.launch() (client startup code)
    ///          |
    ///          v
    /// application tasks start (SOMA)
    ///
    /// Owning arrays and C arrays infer N through the overloads below. A plain
    /// Array view requires explicit N. The two-argument overload uses N == 0.
    /// FreeRTOS heap allocation before heap initialization asserts.
    template<size_t N>
    [[noreturn]] void run( void ( *startup_function ) ( void* ),
                           B_task_memory& startup_task_memory,
                           Array<const Array<uint8_t>> heap_regions )
    {
        ASSERT( startup_function != nullptr );

        Heap::initialize<N>( heap_regions );
        Internal::run_after_heap_initialization( startup_function, startup_task_memory );
    }

    template<typename T, size_t N,
             typename = std::enable_if_t<( N != dynamic_extent ) && std::is_same_v<std::remove_const_t<T>, Array<uint8_t>>>>
    [[noreturn]] void run( void ( *startup_function ) ( void* ),
                           B_task_memory& startup_task_memory,
                           const Array<T, N>& heap_regions )
    {
        run<N>( startup_function,
                startup_task_memory,
                Array<const Array<uint8_t>> { heap_regions } );
    }

    template<size_t N>
    [[noreturn]] void run( void ( *startup_function ) ( void* ),
                           B_task_memory& startup_task_memory,
                           const Array<uint8_t> ( &heap_regions )[N] )
    {
        run<N>( startup_function,
                startup_task_memory,
                Array<const Array<uint8_t>> { heap_regions } );
    }

    [[noreturn]] inline void run( void ( *startup_function ) ( void* ),
                                  B_task_memory& startup_task_memory )
    {
        run<0u>( startup_function, startup_task_memory, {} );
    }
} // namespace SOMA

#endif // RUN_H_
