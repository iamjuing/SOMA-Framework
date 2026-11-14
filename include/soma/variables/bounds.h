// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_VARIABLES_BOUNDS_H_
#define SOMA_VARIABLES_BOUNDS_H_

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "soma/assert.h"

namespace SOMA::Internal
{
    // Shared range rules for bounded variables. Applying the result and notifying
    // observers remain the responsibility of the variable class.
    template<typename T>
    class Bounds
    {
        static_assert( std::is_arithmetic_v<T>, "Bounds only supports arithmetic types!" );

    public:
        enum class Status_e
        {
            rejected,
            accepted,
            clamped
        };

        struct Result
        {
            T value;
            Status_e status;

            bool accepted( void ) const { return status != Status_e::rejected; }
            bool clamped( void ) const { return status == Status_e::clamped; }
        };

        Bounds( T min_value,
                T max_value )
            : m_min_value( min_value ),
              m_max_value( max_value )
        {
            ASSERT( std::isfinite( min_value ) && std::isfinite( max_value ) && min_value <= max_value );
        }

        T min_value( void ) const { return m_min_value; }
        T max_value( void ) const { return m_max_value; }

        bool contains( T value ) const
        {
            return std::isfinite( value ) &&
                   ( value >= m_min_value ) &&
                   ( value <= m_max_value );
        }

        Result normalize( T value ) const
        {
            Result rv_result { value, Status_e::rejected };

            if( std::isfinite( value ) )
            {
                T clamped_value = std::clamp( value, m_min_value, m_max_value );

                rv_result.value = clamped_value;
                rv_result.status = value == clamped_value ? Status_e::accepted : Status_e::clamped;
            }

            return rv_result;
        }

    private:
        T m_min_value;
        T m_max_value;
    };
} // namespace SOMA::Internal

#endif // SOMA_VARIABLES_BOUNDS_H_
