// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/text/string.h"
#include "soma/assert.h"

namespace SOMA
{
    namespace
    {
        const char c_terminator = '\0';
    };

    String::String( Buffer<char> buffer )
        : m_buffer( buffer )
    {
        clear();
    }

    String_view String::view( void ) const
    {
        return String_view( m_buffer.view_items().sub_buffer( 0, length() ) ); // w/o null terminator
    }

    char& String::operator[]( size_t index )
    {
        ASSERT( index < length() );

        Array<char> characters = m_buffer;

        return characters[index];
    }

    char String::operator[]( size_t index ) const
    {
        ASSERT( index < length() );

        return m_buffer.view_items()[index];
    }

    void String::clear( void )
    {
        m_buffer.clear();
        m_buffer.insert( c_terminator );
    }

    bool String::is_empty( void ) const
    {
        return m_buffer.size() == 1u; // null terminator only
    }

    bool String::is_full( void ) const
    {
        return m_buffer.space() == 0u;
    }

    size_t String::capacity( void ) const
    {
        return m_buffer.capacity() - 1;
    }

    size_t String::length( void ) const
    {
        return m_buffer.size() - 1; // exclude the null terminator
    }

    size_t String::space( void ) const
    {
        return m_buffer.space(); // the null terminator is always present
    }

    String& String::operator=( const char* p_C_string )
    {
        clear();

        operator+=( p_C_string );

        return *this;
    }

    String& String::operator=( const String_view& str )
    {
        clear();

        operator+=( str );

        return *this;
    }

    void String::operator+=( const char* p_C_string )
    {
        ASSERT( p_C_string != nullptr );

        size_t src_idx = 0;

        if( space() > 0u )
        {
            m_buffer.displace( 1 ); // remove the null terminator

            while( ( space() > 0u ) && ( p_C_string[src_idx] != c_terminator ) )
            {
                m_buffer.insert( p_C_string[src_idx] );

                ++src_idx;
            }

            if( m_buffer.insert( c_terminator ) == false )
            // truncate
            {
                m_buffer.displace( 1 );
                m_buffer.insert( c_terminator );
            }
        }
    }

    void String::operator+=( const String_view& rhs )
    {
        if( space() > 0u )
        {
            m_buffer.displace( 1 ); // remove the null terminator

            m_buffer.insert( rhs );

            if( m_buffer.insert( c_terminator ) == false )
            // truncate
            {
                m_buffer.displace( 1 );
                m_buffer.insert( c_terminator );
            }
        }
    }

    void String::operator+=( bool value )
    {
        if( value )
        {
            operator+=( "true" );
        }
        else
        {
            operator+=( "false" );
        }
    }

    void String::operator+=( char c )
    {
        if( space() > 0u )
        {
            m_buffer.displace( 1 ); // remove the null terminator

            m_buffer.insert( c );

            m_buffer.insert( c_terminator );
        }
    }

    void String::operator+=( float number )
    {
        format( "%f", number );
    }

    void String::operator+=( double number )
    {
        format( "%lf", number );
    }

    void String::operator-=( size_t count )
    {
        if( is_empty() == false )
        {
            m_buffer.displace( 1 ); // remove the null terminator

            m_buffer.displace( count );

            m_buffer.insert( c_terminator );
        }
    }

    size_t String::format( const char* p_format ... )
    {
        size_t rv_written = 0u;
        va_list args;

        va_start( args, p_format );

        rv_written = vformat( p_format, args );

        va_end( args );

        return rv_written;
    }

    size_t String::vformat( const char* p_format,
                            va_list args )
    {
        size_t rv_written = 0u;

        m_buffer.displace( 1 ); // remove the null terminator

        auto array = m_buffer.get_space();

        int written = vsnprintf( array.storage(), array.size(), p_format, args );

        if( written < 0 )
        // conversion error
        {
            // get to the previous state
            m_buffer.insert( c_terminator );
        }
        else if( static_cast<size_t>( written ) > array.size() )
        // line truncated, the string is full
        {
            m_buffer.place( array.size() ); // includes the null terminator (supplied by vsnprintf)

            rv_written = array.size() - 1; // exclude the null terminator
        }
        else
        {
            m_buffer.place( written + 1 ); // the null terminator (supplied by vsnprintf)

            rv_written = written;
        }

        return rv_written;
    }


    void String::trim_right( void )
    {
        String_view view = this->view().trim_right();
        size_t to_remove = length() - view.length();

        if( to_remove > 0u )
        {
            m_buffer.displace( 1 ); // remove the null terminator

            m_buffer.displace( to_remove );

            m_buffer.insert( c_terminator );
        }
    }

    void String::fraction( uint32_t numerator,
                           uint32_t denominator,
                           const char* p_format,
                           size_t decimal_digit_cnt )
    {
        uint32_t integer = numerator / denominator;
        uint32_t remainder = numerator % denominator;

        uint32_t decimal_shift = 1;

        for( size_t i = 0; i < decimal_digit_cnt; ++i )
        {
            decimal_shift *= 10;
        }

        uint64_t decimal = remainder * decimal_shift;

        decimal += denominator >> 1; // rounding away from 0
        decimal /= denominator;

        if( decimal == decimal_shift )
        {
            integer += 1;
            decimal = 0;
        }

        format( p_format, integer );

        if( decimal_digit_cnt > 0u )
        {
            operator+=( '.' );

            size_t decimals_written = format( "%" PRIu32, static_cast<uint32_t>( decimal ) );

            if( decimals_written < decimal_digit_cnt )
            // prefix with zeros
            {
                operator-=( decimals_written );

                for( size_t i = 0; i < ( decimal_digit_cnt - decimals_written ); ++i )
                {
                    operator+=( '0' );
                }

                format( "%" PRIu32, static_cast<uint32_t>( decimal ) );
            }
        }
    }

    void String::fraction( int32_t numerator,
                           int32_t denominator,
                           const char* p_format,
                           size_t decimal_digit_cnt )
    {
        int32_t integer = numerator / denominator;
        int32_t remainder = numerator % denominator;

        if( remainder < 0 )
        // make remainder always positive
        {
            remainder = ( ~remainder ) + 1;
        }

        if( denominator < 0 )
        // make denominator always positive
        {
            denominator = ( ~denominator ) + 1;
        }

        uint32_t decimal_shift = 1;

        for( size_t i = 0; i < decimal_digit_cnt; ++i )
        {
            decimal_shift *= 10;
        }

        uint64_t decimal = remainder * decimal_shift;

        decimal += denominator >> 1; // rounding away from 0
        decimal /= denominator;

        if( decimal == decimal_shift )
        {
            ( integer < 0 ) ? ( integer -= 1 ) : ( integer += 1 );
            decimal = 0;
        }

        format( p_format, integer );

        if( decimal_digit_cnt > 0u )
        {
            operator+=( '.' );

            size_t decimals_written = format( "%" PRIu32, static_cast<uint32_t>( decimal ) );

            if( decimals_written < decimal_digit_cnt )
            // prefix with zeros
            {
                operator-=( decimals_written );

                for( size_t i = 0; i < ( decimal_digit_cnt - decimals_written ); ++i )
                {
                    operator+=( '0' );
                }

                format( "%" PRIu32, static_cast<uint32_t>( decimal ) );
            }
        }
    }

    bool String::operator==( const char* rhs ) const
    {
        return ( view() == rhs );
    }

    bool String::operator==( const String_view& rhs ) const
    {
        return ( view() == rhs );
    }

    bool String::operator==( const String& rhs ) const
    {
        return ( view() == rhs.view() );
    }

    String::operator String_view() const
    {
        return view();
    }

    const char* String::C_string( void ) const
    {
        return view().buffer().array().storage();
    }
} // namespace SOMA
