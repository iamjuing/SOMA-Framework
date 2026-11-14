// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TASK_STARTUP_EVENT_H_
#define SOMA_TASK_STARTUP_EVENT_H_

#include "soma/observer_handler.h"
#include "soma/assert.h"
#include "soma/runtime/event_task.h"

namespace SOMA
{

    ///
    /// @brief Startup event handler for Event_task that executes callback directly in task context.
    ///
    /// Unlike Task_event, this does not route notifications through the task's event queue.
    /// Instead, it subscribes directly to the task as an observable and executes the callback
    /// immediately when the task starts. This ensures the callback is executed at the very
    /// beginning of task execution, before any queued events are processed.
    ///
    /// Usage:
    ///   Task_startup_event<MyClass> m_startup { task, *this, &MyClass::on_startup };
    ///
    /// @param T type of the object that owns the startup callback
    template<typename T>
    class Task_startup_event
    {
    public:
        ///
        /// @brief ctor with automatic Observer_handler creation and subscription
        /// @param task reference to the event task to subscribe to
        /// @param object reference to the object that will receive the callback
        /// @param callback pointer to the object's method that receives the notification
        Task_startup_event( Event_task& task,
                            T& object,
                            void( T::* callback ) ( void ) );

    private:
        Observer_handler<T> m_observer;
    };

    // Template implementation
    template<typename T>
    Task_startup_event<T>::Task_startup_event( Event_task& task,
                                               T& object,
                                               void( T::* callback ) ( void ) )
        : m_observer( object, callback )
    {
        extern bool is_application_launched( void );
        ASSERT( is_application_launched() == false );
        task.subscribe( m_observer );
    }
} // namespace SOMA

#endif // TASK_STARTUP_EVENT_H_
