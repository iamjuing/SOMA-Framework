// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_B_APPLICATION_H_
#define SOMA_B_APPLICATION_H_

#include <type_traits>
#include <utility>

#include "soma/assert.h"
#include "soma/diagnostics/log.h"
#include "soma/runtime/task.h"
#include "soma/runtime/scheduler_suspension_guard.h"
#include "soma/runtime/task_memory.h"
#include "soma/runtime/heap.h"
#include "soma/text/string_view.h"

namespace SOMA
{

    ///
    /// @brief Base class for an application running on SOMA.
    ///
    ///        - serves as a container for application objects and tasks
    ///        - an image can contain multiple application types, but only the selected application is launched
    ///        - derives from a product-supplied Platform_t
    ///
    ///        The C++ object construction order establishes the application initialization order:
    ///
    ///            Platform_t base -> B_application members -> derived application member -> derived constructor body
    ///
    ///        This ensures lower-level platform services are constructed before the application uses them.
    ///
    ///        Applications are normally created by Application_factory from an OS task after board hardware
    ///        initialization, so construction may use platform and FreeRTOS services.
    ///        Launch closes construction-time registration until this application is destroyed.
    template<typename Platform_t>
    class B_application: public Platform_t
    {
    public:
        static_assert( std::is_class_v<Platform_t>, "Platform_t must be a class type" );
        static_assert( std::is_final_v<Platform_t> == false, "Platform_t must be inheritable" );

        template<typename ... Platform_args>
        explicit B_application( String_view name,
                                Platform_args&& ... platform_args );

        ~B_application( void );

        String_view name( void ) const;
        size_t size_B( void ) const;

        void launch( void );

    private:
        String_view m_name;
        size_t m_size_B = 0u;
        bool m_is_launched = false;

        template<typename ... A>
        friend class Application_factory;
    };

    template<typename Platform_t>
    template<typename ... Platform_args>
    B_application<Platform_t>::B_application( String_view name,
                                              Platform_args&& ... platform_args )
        : Platform_t( std::forward<Platform_args>( platform_args )... ),
          m_name( name )
    {
        // Heap setup is completed by SOMA::run() before the startup task runs.
        extern bool is_application_launched( void );
        ASSERT( is_application_launched() == false );
    }

    template<typename Platform_t>
    B_application<Platform_t>::~B_application( void )
    {
        // Derived application members have already been destroyed.
        if( m_is_launched )
        {
            extern void set_application_launched( bool launched );

            set_application_launched( false );
        }
    }

    template<typename Platform_t>
    String_view B_application<Platform_t>::name( void ) const
    {
        return m_name;
    }

    template<typename Platform_t>
    size_t B_application<Platform_t>::size_B( void ) const
    {
        return m_size_B;
    }

    template<typename Platform_t>
    void B_application<Platform_t>::launch( void )
    {
        extern bool is_application_launched( void );
        extern void set_application_launched( bool launched );

        ASSERT( m_is_launched == false );
        ASSERT( is_application_launched() == false );

        size_t total_stack_capacity = 0u;
        size_t task_count = 0u;
        size_t log_capacity = 0u;

        for( const Task& task: Task::objects() )
        {
            ASSERT( task.is_started() == false );

            total_stack_capacity += task.memory().stack().size_B();
            ++task_count;
        }

        if( Log::get_exposed() != nullptr )
        {
            log_capacity = Log::get_exposed()->capacity();
        }

        ILOG << "[SOMA] ** Launching Application ******************************************";
        ILOG << "[SOMA] * Name:                      " << name();
        ILOG << "[SOMA] * Number of tasks:           " << task_count;
        ILOG << "[SOMA] * Total stacks size:         " << bytes( total_stack_capacity, 1 );
        ILOG << "[SOMA] * Static RAM usage:          " << bytes( size_B(), 1 );

        const Heap::Statistics_t heap = Heap::statistics();

        if( heap.enabled )
        {
            ILOG << "[SOMA] * Initial heap usage:        "
                 << bytes( heap.capacity_B - heap.free_B, 1 )
                 << "/"
                 << bytes( heap.capacity_B, 1 );
        }
        else
        {
            ILOG << "[SOMA] * Initial heap usage:        disabled";
        }
        ILOG << "[SOMA] * Log capacity:              " << bytes( log_capacity, 1 );
        ILOG << "[SOMA] *******************************************************************";

        for( const Task& task: Task::objects() )
        {
            ILOG << "[SOMA] " << Color::green << "Starting task: '" << task.name() << "'" << Color::standard;
        }

        m_is_launched = true;
        set_application_launched( true );

        // creating all the FreeRTOS tasks in suspended state with scheduler disabled to avoid
        // any task preemption during the creation phase
        {
            Scheduler_suspension_guard scheduler_suspension;

            for( Task& task: Task::objects() )
            {
                task.create_task();
                task.resume_task(); // no effect as the scheduler is suspended
            }
        }
    }
} // namespace SOMA

#endif // SOMA_B_APPLICATION_H_
