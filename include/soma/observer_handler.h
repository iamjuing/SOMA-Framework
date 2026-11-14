// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_OBSERVER_HANDLER_H_
#define SOMA_OBSERVER_HANDLER_H_

#include "soma/observer.h"

namespace SOMA
{

    ///
    /// @brief Templated class implementing the observer that passes the generic notification to a specific object/method.
    ///        NOTE: The notification is called in the context of the data producer's thread.
    ///        NOTE: One handler can only be subscribed to a single Variable.
    ///
    ///        WARNING: If the same callback is associated with more handlers, thread safety of the callback must be considered
    ///                 as it can potentially execute from different tasks (e.g. Variable owner and a Variable overloading task).
    ///
    ///        Usage example:
    ///
    ///        class Component
    ///        {
    ///        public:
    ///            Component(Variable<float>& input_variable)
    ///                : m_input_variable(input_variable),
    ///                  m_handler(*this, &Component::callback)
    ///            {
    ///                // intentionally empty
    ///            }
    ///
    ///            void init()
    ///            {
    ///                m_input_variable.subscribe(m_handler);
    ///            }
    ///
    ///        private:
    ///            Variable<float>& m_input_variable;
    ///            Observer_handler<Component> m_handler;
    ///
    ///            void callback( void )
    ///            {
    ///                // Process the new value in two ways:
    ///                //
    ///                // 1) Process right in the callback in (thread) context of the data producer.
    ///                //    This shall be fast and not blocking the data producer (similar to ISR).
    ///                //    Also, take thread-safety into consideration if this class has also a thread, since the callback
    ///                //    executes in the data producer's thread of execution.
    ///                //
    ///                // 2) If this class contains its own thread of execution, postpone processing of the data using proper means:
    ///                //    - if it is not important to process every new value, just the latest one, signal the data change to
    ///                //      the thread, and read the most recent value in the thread.
    ///                //    - push the new value in a queue, and pop it in the thread
    ///            }
    ///        };
    ///
    /// @param O type of the object that owns the handler
    template<typename O>
    class Observer_handler: public Observer
    {
    public:
        // type of a pointer to method of class O
        using Callback_method_t = void ( O::* ) ( void );

        Observer_handler( O& object,
                          Callback_method_t callback );

    private:
        O& m_object;
        Callback_method_t m_callback_method;

        void notify_handler( void ) override;
    };

    ///
    /// @brief ctor
    /// @param object   reference to object that owns the handler
    /// @param callback pointer to the object's method that receives the notification
    template<typename O>
    Observer_handler<O>::Observer_handler( O& object,
                                           Callback_method_t callback )
        : m_object( object ),
          m_callback_method( callback )
    {
        // intentionally empty
    }

    ///
    /// @brief Notifies the owning object in the caller's thread of execution.
    template<typename O>
    void Observer_handler<O>::notify_handler( void )
    {
        ( m_object.*m_callback_method ) ();
    }
} // namespace SOMA

#endif // OBSERVER_HANDLER_H_
