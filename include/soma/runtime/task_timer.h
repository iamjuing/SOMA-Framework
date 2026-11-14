// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TASK_TIMER_H_
#define SOMA_TASK_TIMER_H_

#include "soma/runtime/b_task_timer.h"
#include "soma/observer_handler.h"
#include "soma/runtime/event_task.h"

namespace SOMA
{

    template<typename T>
    class Task_timer: public B_task_timer
    {
    public:
        ///
        /// @brief ctor with automatic Observer_handler creation
        /// @param event_task reference to the event task
        /// @param object reference to the object that will receive the callback
        /// @param callback pointer to the object's method that receives the notification
        Task_timer( Event_task& event_task,
                    T& object,
                    void( T::* callback ) ( void ) );

    private:
        Observer_handler<T> m_observer;
    };

    template<typename T>
    Task_timer<T>::Task_timer( Event_task& event_task,
                               T& object,
                               void( T::* callback ) ( void ) )
        : B_task_timer(),
          m_observer( object, callback )
    {
        event_task.register_timer( *this );
        subscribe( m_observer );
    }
} // namespace SOMA

#endif // TASK_TIMER_H_
