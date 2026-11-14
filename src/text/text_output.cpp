// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cstring>

#include "soma/text/text_output.h"
#include "soma/io/i_blocking_output_stream.h"
#include "soma/variables/b_variable.h"
#include "soma/diagnostics/log.h"

namespace SOMA
{
    Text_output::Text_output( I_blocking_output_stream& output_stream )
        : m_output_stream( output_stream ),
          mp_line_buffer( nullptr )
    {
        // intentionally empty
    }

    Text_output::Text_output( I_blocking_output_stream& output_stream,
                              Buffer<char>& line_buffer )
        : m_output_stream( output_stream ),
          mp_line_buffer( &line_buffer )
    {
        // intentionally empty
    }

    Text_output::~Text_output()
    {
        flush();
    }

    Text_output& Text_output::write( char ch )
    {
        return write( String_view( Buffer_view( Array( &ch, 1 ) ) ) );
    }

    void Text_output::pad( uint8_t cnt,
                           char ch )
    {
        for( size_t i = 0; i < cnt; ++i )
        {
            send( String_view( Buffer_view( Array( &ch, 1 ) ) ) );
        }
    }

    Text_output& Text_output::write( String_view str )
    {
        // apply manipulators
        if( ( m_min_width > 0u ) && ( str.length() < m_min_width ) )
        // padding
        {
            if( m_align_left )
            {
                send( str );
                pad( m_min_width - static_cast<decltype( m_min_width )>( str.length() ), m_pad_character );
            }
            else
            {
                pad( m_min_width - static_cast<decltype( m_min_width )>( str.length() ), m_pad_character );
                send( str );
            }
        }
        else
        {
            send( str );
        }

        // reset manipulators to defaults
        m_min_width = 0;
        m_align_left = false;
        m_print_hex = false;
        m_pad_character = ' ';
        m_precision = mc_default_float_precision; // standard default precision

        return *this;
    }

    void Text_output::send( String_view str )
    {
        if( mp_line_buffer == nullptr )
        // no buffering - write directly to the stream
        {
            m_output_stream.write( str );
        }
        else
        // line buffer available
        {
            for( char ch: str )
            {
                // Try to add character to buffer
                if( mp_line_buffer->insert( ch ) == false )
                {
                    // flush it first
                    flush_line_buffer();

                    mp_line_buffer->insert( ch );
                }

                if( ch == '\n' )
                {
                    flush_line_buffer();
                }
            }
        }
    }

    void Text_output::flush_line_buffer( void )
    {
        if( ( mp_line_buffer != nullptr ) && ( mp_line_buffer->is_empty() == false ) )
        {
            m_output_stream.write( String_view( mp_line_buffer->view_items() ) );

            // Clear the buffer
            mp_line_buffer->clear();
        }
    }

    void Text_output::flush( void )
    {
        flush_line_buffer();
    }

    I_blocking_output_stream& Text_output::blocking_stream( void )
    {
        return m_output_stream;
    }

    uint8_t Text_output::precision( void ) const
    {
        return m_precision;
    }

    Text_output& Text_output::operator<<( const B_variable& P )
    {
        P.to_text( *this );

        return *this;
    }

    Text_output& Text_output::operator<<( const char* p_str )
    {
        return write( p_str );
    }

    Text_output& Text_output::operator<<( const String_view& str )
    {
        return write( str );
    }

    Text_output& Text_output::operator<<( bool value )
    {
        const String_view value_str = value ? "true" : "false";

        return write( value_str );
    }

    Text_output& Text_output::operator<<( Text_output& ( *manipulator ) ( Text_output& ) )
    {
        return manipulator( *this );
    }

    Text_output& Text_output::operator<<( const min_width& min_width )
    {
        m_min_width = min_width.mc_min_width;

        return *this;
    }

    Text_output& Text_output::operator<<( const fill_with& fill_with )
    {
        m_pad_character = fill_with.mc_fill_with;

        return *this;
    }

    Text_output& Text_output::operator<<( const class precision& precision )
    {
        m_precision = precision.mc_precision;

        return *this;
    }

    Text_output& left( Text_output& output )
    {
        output.m_align_left = true;

        return output;
    }

    Text_output& hex( Text_output& output )
    {
        output.m_print_hex = true;

        return output;
    }
} // namespace SOMA
