// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/text/text_string_input.h"
#include "soma/text/string_view.h"
#include "soma/assert.h"

namespace SOMA
{
    Text_string_input::Text_string_input( const String_view& string )
        : Text_input( m_input_string_stream ),
          m_input_string_stream( string )
    {
        // intentionally empty
    }

    bool Text_string_input::is_empty( void )
    {
        return m_input_string_stream.m_position_idx >= m_input_string_stream.m_input_string.length();
    }

    void Text_string_input::return_back( String_view string )
    {
        if( m_input_string_stream.m_position_idx >= string.length() )
        {
            m_input_string_stream.m_position_idx -= string.length();
        }
    }

    void Text_string_input::return_back( [[maybe_unused]] char ch )
    {
        if( m_input_string_stream.m_position_idx > 0 )
        {
            --m_input_string_stream.m_position_idx;
        }
    }

    Text_string_input& Text_string_input::operator>>( String_view& out_string )
    {
        if( m_is_error_active == false )
        {
            char ch = '\0';
            bool character_read = false;

            String_view empty_string;

            out_string = empty_string;

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
                size_t start_idx = m_input_string_stream.m_position_idx - 1; // -1 because the first character was already read
                size_t length = 1;

                while( read( ch ) )
                {
                    if( std::isspace( ch ) )
                    {
                        return_back( ch );
                        break;
                    }

                    ++length;
                }

                out_string = m_input_string_stream.m_input_string.sub_string( start_idx, length );
            }

            if( out_string.is_empty() )
            {
                m_is_error_active = true;
            }
        }

        return *this;
    }

    String_view Text_string_input::get_line( void )
    {
        size_t start_idx = m_input_string_stream.m_position_idx;
        size_t length = m_input_string_stream.m_input_string.length() - start_idx;

        m_input_string_stream.m_position_idx += length;

        return String_view( m_input_string_stream.m_input_string.sub_string( start_idx, length ) );
    }

    String_view Text_string_input::get_line( char delimiter )
    {
        size_t start_idx = m_input_string_stream.m_position_idx;
        size_t length = 0;

        for( size_t i = start_idx; i < m_input_string_stream.m_input_string.length(); ++i )
        {
            ++m_input_string_stream.m_position_idx;

            if( m_input_string_stream.m_input_string.buffer()[i] == delimiter )
            {
                break;
            }

            ++length;
        }

        return String_view( m_input_string_stream.m_input_string.sub_string( start_idx, length ) );
    }

    Text_string_input::Stream::Stream( const String_view& input_string )
        : m_input_string( input_string )
    {
        // intentionally empty
    }

    void Text_string_input::Stream::read( Buffer<uint8_t>& out_buffer )
    {
        if( m_position_idx < m_input_string.length() )
        {
            out_buffer.insert( m_input_string[m_position_idx] );
            ++m_position_idx;
        }
    }

    void Text_string_input::Stream::read( [[maybe_unused]] Buffer<uint8_t>& out_buffer,
                                          [[maybe_unused]] uint32_t timeout_ms )
    {
        ASSERT( false );
    }
} // namespace SOMA
