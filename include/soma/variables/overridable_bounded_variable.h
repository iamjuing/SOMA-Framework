// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_OVERRIDABLE_BOUNDED_VARIABLE_H_
#define SOMA_OVERRIDABLE_BOUNDED_VARIABLE_H_

#include "soma/assert.h"
#include "soma/diagnostics/log.h"
#include "soma/variables/bounds.h"
#include "soma/variables/overridable_variable.h"

namespace SOMA
{

    template<typename T>
    class Overridable_bounded_variable: public Overridable_variable<T>
    {
    public:
        Overridable_bounded_variable( T min_value,
                                      T max_value );
        Overridable_bounded_variable( T value,
                                      T min_value,
                                      T max_value );
        Overridable_bounded_variable( T value,
                                      T min_value,
                                      T max_value,
                                      String_view name );

        void init( T value ) override;

        Overridable_bounded_variable<T>& operator=( T value ) override;
        Overridable_bounded_variable<T>& operator=( const Overridable_bounded_variable<T>& variable );

        using Overridable_variable<T>::set_override;
        bool set_override( T value ) override;

        T min_value( void ) const;
        T max_value( void ) const;

    private:
        Internal::Bounds<T> m_bounds;

        void set( T value );
    };

    template<typename T>
    Overridable_bounded_variable<T>::Overridable_bounded_variable( T min_value,
                                                                   T max_value )
        : Overridable_variable<T>( min_value ),
          m_bounds( min_value, max_value )
    {
        // intentionally empty
    }

    template<typename T>
    Overridable_bounded_variable<T>::Overridable_bounded_variable( T value,
                                                                   T min_value,
                                                                   T max_value )
        : Overridable_variable<T>( value ),
          m_bounds( min_value, max_value )
    {
        ASSERT( m_bounds.contains( value ) );
    }

    template<typename T>
    Overridable_bounded_variable<T>::Overridable_bounded_variable( T value,
                                                                   T min_value,
                                                                   T max_value,
                                                                   String_view name )
        : Overridable_variable<T>( value, name ),
          m_bounds( min_value, max_value )
    {
        ASSERT( m_bounds.contains( value ) );
    }

    template<typename T>
    void Overridable_bounded_variable<T>::init( T value )
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
            ELOG << "[SOMA] Overridable bounded variable init rejected a non-finite value";
        }
    }

    template<typename T>
    Overridable_bounded_variable<T>& Overridable_bounded_variable<T>::operator=( T value )
    {
        set( value );

        return *this;
    }

    template<typename T>
    Overridable_bounded_variable<T>& Overridable_bounded_variable<T>::operator=( const Overridable_bounded_variable<T>& variable )
    {
        if( this != &variable )
        {
            operator=( variable.get() );
        }

        return *this;
    }

    template<typename T>
    T Overridable_bounded_variable<T>::min_value( void ) const
    {
        return m_bounds.min_value();
    }

    template<typename T>
    T Overridable_bounded_variable<T>::max_value( void ) const
    {
        return m_bounds.max_value();
    }

    template<typename T>
    void Overridable_bounded_variable<T>::set( T value )
    {
        auto result = m_bounds.normalize( value );

        if( result.accepted() )
        {
            if( result.clamped() )
            {
                ELOG << "[SOMA] Overridable bounded variable value clamped from " << value << " to " << result.value;
            }

            Overridable_variable<T>::operator=( result.value );
        }
        else
        {
            ELOG << "[SOMA] Overridable bounded variable rejected a non-finite value";
        }
    }


    template<typename T>
    bool Overridable_bounded_variable<T>::set_override( T value )
    {
        bool rv_succeeded = false;

        auto result = m_bounds.normalize( value );

        if( result.accepted() )
        {
            if( result.clamped() )
            {
                ELOG << "[SOMA] Overridable bounded variable override value clamped from " << value << " to " << result.value;
            }

            rv_succeeded = Overridable_variable<T>::set_override( result.value );
        }
        else
        {
            ELOG << "[SOMA] Overridable bounded variable rejected a non-finite override";
        }

        return rv_succeeded;
    }
} // namespace SOMA

#endif // OVERRIDABLE_BOUNDED_VARIABLE_H_
