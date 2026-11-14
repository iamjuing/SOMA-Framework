// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_VARIABLE_OBSERVER_HANDLER_H_
#define SOMA_VARIABLE_OBSERVER_HANDLER_H_

#include "soma/observer_handler.h"
#include "soma/assert.h"
#include "soma/variables/b_variable.h"

namespace SOMA
{

    ///
    /// @brief Templated class implementing Variable observer.
    ///        NOTE: The notification is called in the context of the data producer's thread.
    ///        NOTE: One handler can only be subscribed to a single Variable.
    ///        Subscribe during construction, before concurrent publication begins. The handler does not unsubscribe
    ///        on destruction. Keep it alive until publication has stopped, and keep the variable alive at least as
    ///        long as the handler. Do not mutate the notifying variable from its callback; the callback may run
    ///        while an override holds the variable's lock.
    ///
    ///        Usage example:
    ///
    ///        class Component
    ///        {
    ///        public:
    ///            Component(Variable<float>& input_variable)
    ///                : m_input_variable(input_variable),
    ///                  m_variable_handler(input_variable, *this, &Component::async_callback)
    ///            {
    ///                // intentionally empty
    ///            }
    ///
    ///        private:
    ///            Variable<float>& m_input_variable;
    ///            Variable_observer_handler<Component> m_variable_handler;
    ///
    ///            void async_callback( void )
    ///            {
    ///                // process the new Variable value here if possible or postpone to other thread of interest
    ///            }
    ///        };
    ///
    /// @param O     type of the object that owns the handler
    template<typename O>
    class Variable_observer_handler: public Observer_handler<O>
    {
    public:
        Variable_observer_handler( const B_variable& P,
                                   O& object,
                                   typename Observer_handler<O>::Callback_method_t callback );

        virtual ~Variable_observer_handler() = default;
    };

    ///
    /// @brief ctor
    /// @param P        variable to subscribe to
    /// @param object   reference to object that owns the handler
    /// @param callback pointer to the object's method that receives the notification
    template<typename O>
    Variable_observer_handler<O>::Variable_observer_handler( const B_variable& P,
                                                             O& object,
                                                             typename Observer_handler<O>::Callback_method_t callback )
        : Observer_handler<O>( object, callback )
    {
        extern bool is_application_launched( void );

        ASSERT( is_application_launched() == false );
        P.subscribe( *this );
    }
} // namespace SOMA

#endif // VARIABLE_OBSERVER_HANDLER_H_
