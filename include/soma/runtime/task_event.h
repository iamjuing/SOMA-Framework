// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TASK_EVENT_H_
#define SOMA_TASK_EVENT_H_

#include "soma/runtime/b_task_event.h"
#include "soma/observer_handler.h"
#include "soma/runtime/event_task.h"

namespace SOMA
{
    ///
    /// @brief Helper base class to ensure observer member is initialized before B_task_event
    ///        This is called a "Base-from-member" idiom used in case the derived class member is supplied
    ///        to its base class as a parameters.
    template<typename T>
    struct Task_event_observer_holder
    {
        Observer_handler<T> m_observer;

        Task_event_observer_holder( T & object,
                                    void ( T::* callback ) ( void ) );
    };

    template<typename T>
    Task_event_observer_holder<T>::Task_event_observer_holder( T& object,
                                                               void( T::* callback ) ( void ) )
        : m_observer( object, callback )
    {
        // intentionally empty
    }

    ///
    /// @brief Templated event class that simplifies usage by managing Observer internally.
    ///
    /// Usage with automatic Observer:
    ///   Task_event<MyClass> m_event { task, observable, *this, &MyClass::on_event };
    ///
    /// @param T type of the object that owns the event callback (void for base class usage)
    template<typename T>
    class Task_event: private Task_event_observer_holder<T>, public B_task_event
    {
    public:
        ///
        /// @brief ctor with automatic Observer_handler creation
        /// @param event_task reference to the event task
        /// @param observed_object the object to observe
        /// @param object reference to the object that will receive the callback
        /// @param callback pointer to the object's method that receives the notification
        Task_event( Event_task& event_task,
                    const I_observable& observed_object,
                    T& object,
                    void( T::* callback ) ( void ) );
    };

    template<typename T>
    Task_event<T>::Task_event( Event_task& event_task,
                               const I_observable& observed_object,
                               T& object,
                               void( T::* callback ) ( void ) )
        : Task_event_observer_holder<T>( object, callback ),
          B_task_event( event_task, observed_object, this->m_observer )
    {
        // intentionally empty
    }
} // namespace SOMA

#endif // TASK_EVENT_H_
