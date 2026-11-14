// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_VARIABLE_H_
#define SOMA_VARIABLE_H_

#include <stddef.h>
#include <stdio.h>
#include <atomic>
#include <type_traits>
#include <utility>

#include "soma/variables/b_variable.h"
#include "soma/runtime/scheduler_suspension_guard.h"
#include "soma/text/text_output.h"

namespace SOMA
{
    // Detect the value comparison operators available for T.
    template<typename T, typename = void>
    struct Is_inequality_comparable_t: std::false_type {};

    template<typename T>
    struct Is_inequality_comparable_t<T,
                                    std::void_t<decltype( std::declval<T&>() != std::declval<T&>() )>>: std::bool_constant<std::is_convertible_v<decltype( std::declval<T&>() != std::declval<T&>() ), bool>> {};

    template<typename T, typename = void>
    struct Is_equality_comparable_t: std::false_type {};

    template<typename T>
    struct Is_equality_comparable_t<T,
                                  std::void_t<decltype( std::declval<T&>() == std::declval<T&>() )>>: std::bool_constant<std::is_convertible_v<decltype( std::declval<T&>() == std::declval<T&>() ), bool>> {};

    ///
    /// @brief Templated class representing a typed, non-overridable variable.
    ///        T must be trivially copyable, default-constructible and copyable. An explicit initial value is required.
    ///        Changes notify observers when T compares unequal. T must provide == or !=; scoped enums have both
    ///        operators built in.
    ///        Normal assignment belongs to one owner task. Other tasks may read with get(), but variable access
    ///        is not supported from an ISR. Non-atomic value access suspends task scheduling on the current core;
    ///        this does not synchronize another core.
    ///        Notifications run synchronously in the writing task. Subscribe before concurrent access starts,
    ///        and keep observers alive until publication has stopped.
    ///
    /// @param T type of the Variable value
    template<typename T>
    class Variable: public B_variable
    {
        static_assert( std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T> &&
                       std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>,
                       "Variable requires a trivially copyable, default-constructible, copyable value type" );
        static_assert( Is_inequality_comparable_t<T>::value || Is_equality_comparable_t<T>::value,
                       "Variable value type requires operator== or operator!=" );

    public:
        explicit Variable( T value );

        Variable( T value,
                  String_view name );

        template<typename U, typename = std::enable_if_t<std::is_constructible_v<T, U> &&
                                                         !std::is_same_v<std::decay_t<U>, T> &&
                                                         !std::is_base_of_v<B_variable, std::decay_t<U>>>>
        explicit Variable( U&& value );

        Variable( const Variable<T>& rhs ) = delete;

        Variable<T>& operator=( const Variable<T>& variable );

        virtual void init( T value );

        constexpr size_t size( void );

        virtual Variable<T>& operator=( T value );

        bool operator==( T value ) const
        {
            return get() == value;
        }

        bool operator!=( T value ) const
        {
            return operator==( value ) == false;
        }

        operator T() const;
        T get( void ) const;

        virtual bool set_override( T value );

        // B_variable overrides
        void to_text( Text_output& output ) const override;
        bool set_override( String_view value ) override;

    private:
        using Maybe_atomic_t = typename std::conditional<std::atomic<T>::is_always_lock_free, std::atomic<T>, T>::type;

        // "Maybe_atomic_t = atomic<T>" if T is always lock free so it does not require a critical section
        // when being accessed; otherwise "Maybe_atomic_t = T" and critical section is applied (see the value accessors).
        // NOTE: Metafunction must be used as using std::atomic<T> unconditionally might result in compilation error
        //       for types (or structures) not supported by the standard library.
        Maybe_atomic_t m_value;

        void set( T value );
    };

    ///
    /// @brief Ctor.
    /// @param value initial value of the variable
    template<typename T>
    Variable<T>::Variable( T value )
        : m_value( value )
    {
        // intentionally empty
    }

    template<typename T>
    Variable<T>::Variable( T value,
                           String_view name )
        : B_variable( name ),
          m_value( value )
    {
        // intentionally empty
    }

    ///
    /// @brief Ctor allowing construction from any type U constructible to T.
    /// @param value initial value convertible to T
    template<typename T>
    template<typename U, typename>
    Variable<T>::Variable( U&& value )
        : m_value( T( std::forward<U>( value ) ) )
    {
        // intentionally empty
    }

    ///
    /// @brief Converts and appends the string representation of the value to the provided string.
    /// @param str string where the value is appended
    template<typename T>
    void Variable<T>::to_text( Text_output& output ) const
    {
        output << get();
    }

    ///
    /// @brief Sets a loaded value without notifying registered observers.
    ///        Call this before publishing the variable to consumers or starting concurrent access or overrides.
    ///        This is useful when the value (e.g. from persistent storage) is only available in the owner's constructor body.
    /// @param value new variable value
    template<typename T>
    void Variable<T>::init( T value )
    {
        m_value = value;
    }

    ///
    /// @brief Assignment operator for setting the variable value.
    /// @param value new variable value
    /// @retval reference to this variable
    template<typename T>
    Variable<T>& Variable<T>::operator=( T value )
    {
        set( value );

        return *this;
    }

    ///
    /// @brief Assigns only the current value of another variable.
    /// @param variable source of the new value
    /// @retval reference to this variable
    template<typename T>
    Variable<T>& Variable<T>::operator=( const Variable<T>& variable )
    {
        return operator=( variable.get() );
    }

    ///
    /// @brief Conversion operator for getting variable value.
    /// @retval variable value
    template<typename T>
    Variable<T>::operator T() const
    {
        return get();
    }

    ///
    /// @brief Sets a new variable value and notifies the observer list if the value has changed.
    /// @param value new variable value
    template<typename T>
    void Variable<T>::set( T value )
    {
        T old_value;

        if constexpr( std::is_same_v<Maybe_atomic_t, std::atomic<T>> )
        // the data type is atomic
        {
            old_value = m_value.exchange( value );
        }
        else
        {
            Scheduler_suspension_guard scheduler_suspension;

            old_value = m_value;
            m_value = value;
        }

        bool changed;

        if constexpr( Is_inequality_comparable_t<T>::value )
        {
            changed = ( old_value != value );
        }
        else
        {
            changed = !( old_value == value );
        }

        if( changed )
        {
            notify();
        }
    }

    ///
    /// @brief Retrieves variable value.
    /// @retval variable value
    template<typename T>
    T Variable<T>::get( void ) const
    {
        T rv_value;

        if constexpr( std::is_same_v<Maybe_atomic_t, std::atomic<T>> )
        // the data type is atomic
        {
            rv_value = m_value;
        }
        else
        {
            Scheduler_suspension_guard scheduler_suspension;

            rv_value = m_value;
        }

        return rv_value;
    }

    ///
    /// @brief Retrieves storage size of the variable value.
    /// @retval storage size of the variable value in bytes
    template<typename T>
    constexpr size_t Variable<T>::size( void )
    {
        return sizeof( T );
    }

    ///
    /// @brief Overrides variable value.
    /// @param value to override the variable value with
    /// @retval true, if the value has been overridden
    ///         false, if not supported
    template<typename T>
    bool Variable<T>::set_override( [[maybe_unused]] T value )
    {
        ELOG << "[SOMA] The variable is not overridable!";

        return false;
    }

    template<typename T>
    bool Variable<T>::set_override( String_view value )
    {
        return B_variable::set_override( value );
    }
} // namespace SOMA

#endif // VARIABLE_H_
