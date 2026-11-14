// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BOUNDED_VARIABLE_H_
#define SOMA_BOUNDED_VARIABLE_H_

#include "soma/assert.h"
#include "soma/diagnostics/log.h"
#include "soma/variables/bounds.h"
#include "soma/variables/variable.h"

namespace SOMA
{

    template<typename T>
    class Bounded_variable: public Variable<T>
    {
    public:
        Bounded_variable( T min_value,
                          T max_value );
        Bounded_variable( T value,
                          T min_value,
                          T max_value );
        Bounded_variable( T value,
                          T min_value,
                          T max_value,
                          String_view name );
        Bounded_variable( const Bounded_variable<T>& rhs ) = delete;

        void init( T value ) override;

        Bounded_variable<T>& operator=( T value ) override;
        Bounded_variable<T>& operator=( const Bounded_variable<T>& variable );

        T min_value( void ) const;
        T max_value( void ) const;

    private:
        Internal::Bounds<T> m_bounds;

        void set( T value );
    };

    template<typename T>
    Bounded_variable<T>::Bounded_variable( T min_value,
                                           T max_value )
        : Variable<T>( min_value ),
          m_bounds( min_value, max_value )
    {
        // intentionally empty
    }

    template<typename T>
    Bounded_variable<T>::Bounded_variable( T value,
                                           T min_value,
                                           T max_value )
        : Variable<T>( value ),
          m_bounds( min_value, max_value )
    {
        ASSERT( m_bounds.contains( value ) );
    }

    template<typename T>
    Bounded_variable<T>::Bounded_variable( T value,
                                           T min_value,
                                           T max_value,
                                           String_view name )
        : Variable<T>( value, name ),
          m_bounds( min_value, max_value )
    {
        ASSERT( m_bounds.contains( value ) );
    }

    template<typename T>
    void Bounded_variable<T>::init( T value )
    {
        auto result = m_bounds.normalize( value );

        if( result.accepted() )
        {
            if( result.clamped() )
            {
                ELOG << "[SOMA] Bounded variable init value clamped from " << value << " to " << result.value;
            }

            Variable<T>::init( result.value );
        }
        else
        {
            ELOG << "[SOMA] Bounded variable init rejected a non-finite value";
        }
    }

    template<typename T>
    Bounded_variable<T>& Bounded_variable<T>::operator=( T value )
    {
        set( value );

        return *this;
    }

    template<typename T>
    Bounded_variable<T>& Bounded_variable<T>::operator=( const Bounded_variable<T>& variable )
    {
        return operator=( variable.get() );
    }

    template<typename T>
    T Bounded_variable<T>::min_value( void ) const
    {
        return m_bounds.min_value();
    }

    template<typename T>
    T Bounded_variable<T>::max_value( void ) const
    {
        return m_bounds.max_value();
    }

    template<typename T>
    void Bounded_variable<T>::set( T value )
    {
        auto result = m_bounds.normalize( value );

        if( result.accepted() )
        {
            if( result.clamped() )
            {
                ELOG << "[SOMA] Bounded variable value clamped from " << value << " to " << result.value;
            }

            Variable<T>::operator=( result.value );
        }
        else
        {
            ELOG << "[SOMA] Bounded variable rejected a non-finite value";
        }
    }
} // namespace SOMA

#endif // BOUNDED_VARIABLE_H_
