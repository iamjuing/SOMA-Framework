// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_B_VARIABLE_H_
#define SOMA_B_VARIABLE_H_

#include <type_traits>

#include "soma/assert.h"
#include "soma/diagnostics/log.h"
#include "soma/config.h"
#include "soma/text/string.h"
#include "soma/text/string_view.h"
#include "soma/i_observable.h"
#include "soma/i_object_enumerator.h"
#include "soma/text/text_output.h"

namespace SOMA
{
    // Custom types may provide a from_text(String_view, T&) overload in their own namespace.
    // A false result indicates invalid input; the output argument may have been modified.
    inline bool from_text( String_view value_string,
                           bool& out_value )
    {
        bool rv_succeeded = true;

        if( value_string == "true" )
        {
            out_value = true;
        }
        else if( value_string == "false" )
        {
            out_value = false;
        }
        else
        {
            rv_succeeded = false;
        }

        return rv_succeeded;
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    bool from_text( String_view value_string,
                    T& out_value )
    {
        return value_string.to( out_value );
    }

    template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    bool from_text( String_view value_string,
                    T& out_value )
    {
        return value_string.to( out_value );
    }

    ///
    /// @brief Type independent base class representing a global variable the software components can use to exchange data,
    ///        while keeping them independent of each other.
    ///        - provides notification subscriptions API
    ///        - provides API for overriding the value from a string
    ///        - provides API for converting the value to a string
    ///
    ///        Custom value types provide operator<< for output and, when overridable, from_text() for parsing.
    ///
    ///        Note:
    ///        - child class implementing value overriding API is supposed to mutually exclude multiple writers changing
    ///          the value and notifying the observers
    ///        - observer subscription is not thread-safe; establish subscriptions before concurrent publication and
    ///          keep observers alive until publication stops. Unsubscribe only while notification is quiescent.
    ///
    /// @param T type of the Variable value
    class B_variable: public I_observable, public I_object_enumerator<B_variable>
    {
    public:
        virtual ~B_variable() = default;

        virtual void to_text( Text_output& str ) const = 0;

        virtual bool set_override( String_view value );
        virtual void clear_override( void );

        virtual bool is_overridden( void );
        virtual bool is_overridable( void );

        String_view name( void ) const;

    protected:
        B_variable( void ) = default;
        explicit B_variable( String_view name );

    private:
#ifdef SOMA_CONFIG_ENABLE_NAMED_VARIABLES
        String_view m_name;
#endif
    };

    inline B_variable::B_variable( [[maybe_unused]] String_view name )
#ifdef SOMA_CONFIG_ENABLE_NAMED_VARIABLES
        : m_name( name )
#endif
    {
    }

    inline String_view B_variable::name( void ) const
    {
        String_view rv_name;

#ifdef SOMA_CONFIG_ENABLE_NAMED_VARIABLES

        rv_name = m_name;
#endif

        return rv_name;
    }

    inline bool B_variable::set_override( [[maybe_unused]] String_view value )
    {
        ELOG << "[SOMA] The variable is not overridable!";

        return false;
    }

    inline void B_variable::clear_override( void )
    {
        // intentionally empty
    }

    inline bool B_variable::is_overridden( void )
    {
        return false;
    }

    inline bool B_variable::is_overridable( void )
    {
        return false;
    }
} // namespace SOMA

#endif // SOMA_B_VARIABLE_H_
