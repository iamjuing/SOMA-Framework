// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_OVERRIDABLE_VARIABLE_H_
#define SOMA_OVERRIDABLE_VARIABLE_H_

#include "soma/variables/variable.h"
#include "soma/runtime/sleeping_spin_lock.h"
#include "soma/runtime/sleeping_spin_lock_guard.h"

namespace SOMA
{

    ///
    /// @brief Class representing an overridable extension to simple Variable class. Since the value can be overriden from
    ///        tasks different from the owner's task, a synchronization mechanism must be aplied.
    ///        Spinlock synchronization mechanism was preferred over semaphore/mutex to keep the class footprint small.
    ///
    ///        The owner is the only task assigning normal values. Multiple maintenance/test tasks may override or clear.
    ///        Each override transition and its observer notifications are serialized with owner assignments.
    ///        A notification runs synchronously in the caller's task, which can be the owner or an overrider.
    ///        Reading the value and override status separately does not yield an atomic snapshot of both.
    ///
    ///        WARNING:
    ///        Overrides should be rare. A contending task sleeps one OS tick per failed lock attempt, so the total
    ///        delay is not bounded to one tick. Callbacks must be short and nonblocking. They must not mutate this
    ///        variable or reenter its lock through a cycle.
    ///
    /// @param T type of the Variable value
    template<typename T>
    class Overridable_variable: public Variable<T>
    {
    public:
        explicit Overridable_variable( T value );

        Overridable_variable( T value,
                              String_view name );

        Overridable_variable<T>& operator=( T value ) override;
        Overridable_variable<T>& operator=( const Overridable_variable<T>& variable );

        bool set_override( T value ) override;
        bool set_override( String_view value ) final;
        void clear_override( void ) final;

        bool is_overridden( void ) final;
        bool is_overridable( void ) final;

    private:
        // This is the owner value while the variable is overridden. Access is protected by m_spin_lock.
        T m_original_value;
        // Readers may query the status without taking the sleeping lock.
        std::atomic_bool m_is_overridden { false };

        // As the overriding API in fact sets up multiple owners, the writing access needs to be a critical section.
        // NOTE: Sleeping spin lock is used instead of OS primitives as it has a minimal memory footprint. The cost for
        //       this is a rare side effect of delaying the owner by one OS tick per retry. This feature
        //       is primarily targeted for testing and maintenance.
        Sleeping_spin_lock m_spin_lock;

        void set( T value );
    };

    ///
    /// @brief ctor
    /// @param value initial value of the variable
    template<typename T>
    Overridable_variable<T>::Overridable_variable( T value )
        : Variable<T>( value ),
          m_original_value( value )
    {
        // intentionally empty
    }

    template<typename T>
    Overridable_variable<T>::Overridable_variable( T value,
                                                   String_view name )
        : Variable<T>( value, name ),
          m_original_value( value )
    {
        // intentionally empty
    }

    ///
    /// @brief Assignment operator for setting the variable value.
    /// @param value new variable value
    /// @retval reference to this variable
    template<typename T>
    Overridable_variable<T>& Overridable_variable<T>::operator=( T value )
    {
        set( value );

        return *this;
    }

    ///
    /// @brief Assigns only the current value of another variable.
    /// @param variable source of the new value
    /// @retval reference to this variable
    template<typename T>
    Overridable_variable<T>& Overridable_variable<T>::operator=( const Overridable_variable<T>& variable )
    {
        if( this != &variable )
        {
            operator=( variable.get() );
        }

        return *this;
    }

    ///
    /// @brief Sets a new variable value.
    ///        The new value is remembered, however, not applied if the override is active.
    /// @param value new variable value
    /// @retval true, if the value has changed
    template<typename T>
    void Overridable_variable<T>::set( T value )
    {
        Sleeping_spin_lock_guard lock_guard( m_spin_lock );

        if( m_is_overridden.load() )
        // save the new value while override is active
        {
            m_original_value = value;
        }
        else
        {
            Variable<T>::operator=( value );
        }
    }

    ///
    /// @brief Overrides variable value.
    /// @param value to override the variable value with
    /// @retval true, if parsing and the typed override succeeded; false for invalid input
    template<typename T>
    bool Overridable_variable<T>::set_override( T value )
    {
        Sleeping_spin_lock_guard lock_guard( m_spin_lock );

        if( m_is_overridden.load() == false )
        {
            // remember the original variable value
            m_original_value = Variable<T>::get();

            m_is_overridden.store( true );
        }

        // Keep publication and notification serialized with other override operations.
        Variable<T>::operator=( value );

        return true;
    }

    ///
    /// @brief Overrides variable value.
    /// @param value to override the variable value with
    /// @retval true, if the value has been overridden
    ///         false, otherwise
    template<typename T>
    bool Overridable_variable<T>::set_override( String_view value )
    {
        bool rv_success = true;

        T new_value;

        rv_success = rv_success && from_text( value, new_value );
        rv_success = rv_success && set_override( static_cast<T>( new_value ) );

        return rv_success;
    }

    ///
    /// @brief Cancels variable value override and retrieves the original value.
    template<typename T>
    void Overridable_variable<T>::clear_override( void )
    {
        Sleeping_spin_lock_guard lock_guard( m_spin_lock );

        if( m_is_overridden.load() )
        {
            // Callbacks observing the restored value must see the cleared status.
            m_is_overridden.store( false );

            Variable<T>::operator=( m_original_value );
        }
    }

    ///
    /// @brief Checks whether the variable value is currently overridden.
    /// @retval true, if variable value is currently overridden
    template<typename T>
    bool Overridable_variable<T>::is_overridden( void )
    {
        return m_is_overridden.load();
    }

    ///
    /// @brief Retrieves whether the variable is overridable.
    /// @retval true, if variable value is overridable
    template<typename T>
    bool Overridable_variable<T>::is_overridable( void )
    {
        return true;
    }
} // namespace SOMA

#endif // OVERRIDABLE_VARIABLE_H_
