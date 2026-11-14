// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <type_traits>
#include <cstdint>
#include <cctype>

#include "soma/text/text_input.h"
#include "soma/text/string_view.h"
#include "soma/io/i_blocking_input_stream.h"

namespace SOMA
{
    Text_input::Text_input( I_blocking_input_stream& input_stream )
        : m_input_stream( input_stream )
    {
        // intentionally empty
    }

    void Text_input::return_back( String_view string )
    {
        m_returned_str.clear();
        m_returned_str = string;
    }

    void Text_input::return_back( char ch )
    {
        m_returned_str.clear();
        m_returned_str += ch;
    }

    bool Text_input::read( char& out_char )
    {
        bool rv_succeded = false;

        if( m_returned_str.is_empty() )
        {
            if( is_empty() == false )
            {
                auto read_buffer = Buffer( Array( reinterpret_cast<uint8_t*>( &out_char ), 1 ) );

                m_input_stream.read( read_buffer );

                rv_succeded = true;
            }
        }
        else
        {
            out_char = m_returned_str.view()[m_number_string_position];
            ++m_number_string_position;

            if( m_number_string_position >= m_returned_str.length() )
            {
                m_returned_str.clear();
                m_number_string_position = 0;
            }

            rv_succeded = true;
        }

        return rv_succeded;
    }

    bool Text_input::is_empty( void )
    {
        return false;
    }

    I_blocking_input_stream& Text_input::blocking_stream( void )
    {
        return m_input_stream;
    }

    Text_input& Text_input::operator>>( String& out_text )
    {
        if( m_is_error_active == false )
        {
            char ch = '\0';
            bool character_read = false;

            out_text.clear();

            // trim leading white spaces
            while( read( ch ) )
            {
                if( std::isspace( ch ) == 0 )
                {
                    character_read = true;
                    break;
                }
            }

            if( character_read )
            {
                out_text += ch;

                while( read( ch ) )
                {
                    if( std::isspace( ch ) || out_text.is_full() )
                    {
                        return_back( ch );
                        break;
                    }
                    else
                    {
                        out_text += ch;
                    }
                }
            }

            if( out_text.is_empty() )
            {
                m_is_error_active = true;
            }
        }

        return *this;
    }

    Text_input::operator bool() const
    {
        bool rv_is_error_inactive = ( m_is_error_active == false );

        m_is_error_active = false;

        return rv_is_error_inactive;
    }
} // namespace SOMA
