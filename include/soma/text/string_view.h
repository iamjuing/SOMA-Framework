// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_STRING_VIEW_H_
#define SOMA_STRING_VIEW_H_

#include <cctype>
#include <limits>
#include <type_traits>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <cinttypes>
#include <cmath>
#include <cstring>

#include "soma/reverse_iterator.h"
#include "soma/containers/buffer_view.h"
#include "soma/assert.h"
#include "soma/array.h"

namespace SOMA
{

    class String_view
    {
    public:
        using Iterator_t = typename Buffer_view<char>::Iterator_t;

        constexpr String_view( const char* p_C_string );
        constexpr String_view( const char* p_C_string,
                               size_t length );
        constexpr String_view( Buffer_view<char> buffer );
        constexpr String_view( Buffer_view<uint8_t> buffer );
        constexpr String_view( void ) = default;

        constexpr operator Buffer_view<char>( ) const;
        constexpr operator Buffer_view<uint8_t>( ) const;

        constexpr Iterator_t begin( void ) const;
        constexpr Iterator_t end( void ) const;

        constexpr bool is_empty( void ) const;
        constexpr size_t length( void ) const;

        constexpr char operator[]( size_t index ) const;

        constexpr String_view& operator=( const String_view& str ) = default;

        constexpr bool operator==( const char* p_C_string ) const;
        constexpr bool operator==( const String_view& rhs ) const;
        constexpr bool operator!=( const char* p_C_string ) const;
        constexpr bool operator!=( const String_view& rhs ) const;

        constexpr bool to( float& value ) const;
        constexpr bool to( double& value ) const;

        template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_signed<T>>, int> = 0>
        constexpr bool to( T& out_value,
                           size_t base = 0 ) const;

        template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>, int> = 0>
        constexpr bool to( T& out_value,
                           size_t base = 0 ) const;

        constexpr bool to_hex_bytes( Array<uint8_t> output ) const;

        constexpr bool starts_with( char character ) const;
        constexpr bool starts_with( const char* p_C_string ) const;
        constexpr bool starts_with( String_view string ) const;

        constexpr bool ends_with( char character ) const;
        constexpr bool ends_with( const char* p_C_string ) const;
        constexpr bool ends_with( String_view string ) const;

        constexpr String_view trim( void ) const;
        constexpr String_view trim_left( void ) const;
        constexpr String_view trim_right( void ) const;

        constexpr bool find( String_view sub_string,
                             size_t& position ) const;
        constexpr bool find( char character,
                             size_t& position ) const;

        constexpr bool find_last( char character,
                                  size_t& position ) const;

        constexpr String_view sub_string( size_t start_idx,
                                          size_t cnt = std::numeric_limits<size_t>::max() ) const;

        constexpr bool is_single_word() const;

        constexpr Buffer_view<char> buffer( void ) const;

    private:
        Buffer_view<char> m_buffer_view;

        template<typename T>
        static constexpr bool parse_finite_decimal( String_view value,
                                                    T& out_value );

        constexpr size_t detect_base_and_strip_prefix( String_view& out_number_str,
                                                       size_t expected_base ) const;

        constexpr bool char_to_digit( char ch,
                                      size_t base,
                                      uint8_t& digit_value ) const;

        // Forbid implicit conversion to (const) char*, to avoid accidentally comparing pointers
        // instead of strings when using operator==().
        // NOTE: Must be declared 'explicit' as the compiler tries to use it even though it is marked deleted
        //       and generates a warning (ambiguous conversion).
        explicit constexpr operator const char*() const = delete;
        explicit constexpr operator char*() const = delete;
    };

    constexpr String_view::String_view( const char* p_C_string )
    {
        size_t i = 0;

        while( p_C_string[i] != '\0' )
        {
            ++i;
        }

        m_buffer_view = Buffer_view( Array( p_C_string, i ) );
    }

    constexpr String_view::String_view( const char* p_C_string,
                                        size_t length )
        : m_buffer_view( Buffer_view( Array( p_C_string, length ) ) )
    {
        // intentionally empty
    }

    constexpr String_view::String_view( Buffer_view<char> buffer )
        : m_buffer_view( buffer )
    {
        // intentionally empty
    }

    constexpr String_view::String_view( Buffer_view<uint8_t> buffer )
        : m_buffer_view( Array<const char>( static_cast<const char*>( static_cast<const void*>( buffer.array().storage() ) ),
                                            buffer.size() ) )
    {
        // intentionally empty
    }

    constexpr String_view::operator Buffer_view<char>( ) const
    {
        return m_buffer_view;
    }

    constexpr String_view::operator Buffer_view<uint8_t>( ) const
    {
        return Buffer_view( Array( static_cast<const uint8_t*>( static_cast<const void*>( m_buffer_view.array().storage() ) ),
                                   length() ) );
    }

    template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_signed<T>>, int>>
    constexpr bool String_view::to( T& out_value,
                                    size_t base ) const
    {
        String_view number_str = trim_left();
        bool rv_succeeded = ( number_str.is_empty() == false );
        bool is_negative = false;

        if( rv_succeeded && ( number_str[0] == '-' ) )
        {
            is_negative = true;
            number_str = number_str.sub_string( 1 );
        }
        else if( rv_succeeded && ( number_str[0] == '+' ) )
        {
            number_str = number_str.sub_string( 1 );
        }
        else
        {
            // intentionally empty
        }

        rv_succeeded = rv_succeeded && ( number_str.is_empty() == false );

        if( rv_succeeded )
        {
            base = detect_base_and_strip_prefix( number_str, base );
        }

        rv_succeeded = rv_succeeded && ( number_str.is_empty() == false );
        rv_succeeded = rv_succeeded && ( base >= 2u && base <= 36u );

        if( rv_succeeded )
        {
            using Unsigned_t = std::make_unsigned_t<T>;

            const Unsigned_t base_value = static_cast<Unsigned_t>( base );
            const Unsigned_t limit = is_negative
                                          ? Unsigned_t( 0 ) - static_cast<Unsigned_t>( std::numeric_limits<T>::min() )
                                          : static_cast<Unsigned_t>( std::numeric_limits<T>::max() );
            Unsigned_t result = 0;

            for( char ch: number_str )
            {
                uint8_t digit_value = 0;

                rv_succeeded = rv_succeeded && char_to_digit( ch, base, digit_value );

                const Unsigned_t digit = static_cast<Unsigned_t>( digit_value );

                rv_succeeded = rv_succeeded && ( result <= ( limit - digit ) / base_value ); // check overflow

                if( rv_succeeded )
                {
                    result = result * base_value + digit;
                }
                else
                {
                    break;
                }
            }

            if( rv_succeeded )
            {
                if( is_negative && ( result == limit ) )
                {
                    out_value = std::numeric_limits<T>::min();
                }
                else
                {
                    const T magnitude = static_cast<T>( result );

                    out_value = is_negative ? -magnitude : magnitude;
                }
            }
        }

        return rv_succeeded;
    }

    template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>, int>>
    constexpr bool String_view::to( T& out_value,
                                    size_t base ) const
    {
        String_view number_str = trim_left();
        bool rv_succeeded = ( number_str.is_empty() == false );

        rv_succeeded = rv_succeeded && ( number_str[0] != '-' );

        if( rv_succeeded && ( number_str[0] == '+' ) )
        {
            number_str = number_str.sub_string( 1 );
        }

        rv_succeeded = rv_succeeded && ( number_str.is_empty() == false );

        if( rv_succeeded )
        {
            base = detect_base_and_strip_prefix( number_str, base );
        }

        rv_succeeded = rv_succeeded && ( number_str.is_empty() == false );
        rv_succeeded = rv_succeeded && ( base >= 2u && base <= 36u );

        if( rv_succeeded )
        {
            T result = 0;

            for( char ch: number_str )
            {
                uint8_t digit_value = 0;

                rv_succeeded = rv_succeeded && char_to_digit( ch, base, digit_value );

                const T digit = static_cast<T>( digit_value );

                rv_succeeded = rv_succeeded && ( result <= ( std::numeric_limits<T>::max() - digit ) / static_cast<T>( base ) ); // check overflow

                if( rv_succeeded )
                {
                    result = result * base;
                    result += digit;
                }
                else
                {
                    break;
                }
            }

            if( rv_succeeded )
            {
                out_value = result;
            }
        }

        return rv_succeeded;
    }

    constexpr String_view::Iterator_t String_view::begin( void ) const
    {
        return m_buffer_view.begin();
    }

    constexpr String_view::Iterator_t String_view::end( void ) const
    {
        return m_buffer_view.end();
    }

    constexpr bool String_view::operator==( const char* p_C_string ) const
    {
        // terminated at the right place (strings are not necessarily equal though)
        bool rv_equal = ( p_C_string[length()] == '\0' );

        if( rv_equal )
        {
            for( size_t i = 0; i < length(); ++i )
            {
                if( m_buffer_view[i] != p_C_string[i] )
                {
                    rv_equal = false;
                    break;
                }
            }
        }

        return rv_equal;
    }

    constexpr bool String_view::operator==( const String_view& rhs ) const
    {
        return m_buffer_view == rhs.m_buffer_view;
    }

    constexpr bool String_view::operator!=( const char* p_C_string ) const
    {
        return ( operator==( p_C_string ) == false );
    }

    constexpr bool String_view::operator!=( const String_view& rhs ) const
    {
        return ( operator==( rhs ) == false );
    }

    template<typename T>
    constexpr bool String_view::parse_finite_decimal( String_view value,
                                                      T& out_value )
    {
        constexpr size_t max_token_length = 128;
        char buffer[max_token_length] = {};
        const size_t length = value.length();
        bool rv_succeeded = length < max_token_length;

        if( rv_succeeded )
        {
            for( size_t i = 0; i < length; ++i )
            {
                buffer[i] = value[i];
            }
        }

        T parsed_value = 0;
        char* p_end = nullptr;

        if( rv_succeeded )
        {
            if constexpr( std::is_same_v<T, float> )
            {
                parsed_value = strtof( buffer, &p_end );
            }
            else
            {
                parsed_value = strtod( buffer, &p_end );
            }

            size_t consumed = static_cast<size_t>( p_end - buffer );

            while( consumed < length && std::isspace( static_cast<unsigned char>( buffer[consumed] ) ) )
            {
                ++consumed;
            }

            const T max_value = std::numeric_limits<T>::max();

            rv_succeeded = ( p_end != buffer ) && ( consumed == length ) &&
                           ( parsed_value <= max_value ) && ( parsed_value >= -max_value );
        }

        if( rv_succeeded )
        {
            out_value = parsed_value;
        }

        return rv_succeeded;
    }

    constexpr bool String_view::to( float& out_value ) const
    {
        return parse_finite_decimal( *this, out_value );
    }

    constexpr bool String_view::to( double& out_value ) const
    {
        return parse_finite_decimal( *this, out_value );
    }

    constexpr bool String_view::is_empty( void ) const
    {
        return length() == 0u;
    }

    constexpr size_t String_view::length( void ) const
    {
        return m_buffer_view.size();
    }

    constexpr bool String_view::starts_with( char character ) const
    {
        bool rv_starts_with = false;

        if( is_empty() == false )
        {
            auto it = begin();

            if( ( *it ) == character )
            {
                rv_starts_with = true;
            }
        }

        return rv_starts_with;
    }

    constexpr bool String_view::starts_with( const char* p_C_string ) const
    {
        bool rv_starts_with = false;
        size_t i = 0;

        while( ( p_C_string[i] != '\0' ) && ( i < length() ) )
        {
            if( p_C_string[i] != m_buffer_view[i] )
            {
                break;
            }

            ++i;
        }

        if( p_C_string[i] == '\0' )
        // all characters compared
        {
            rv_starts_with = true;
        }

        return rv_starts_with;
    }

    constexpr bool String_view::starts_with( String_view str ) const
    {
        bool rv_starts_with = true;
        size_t i = 0;

        if( str.length() <= length() )
        // str can not be longer than the this string
        {
            while( i < str.length() )
            {
                if( str.m_buffer_view[i] != m_buffer_view[i] )
                {
                    rv_starts_with = false;
                    break;
                }

                ++i;
            }
        }
        else
        {
            rv_starts_with = false;
        }

        return rv_starts_with;
    }

    constexpr bool String_view::ends_with( char character ) const
    {
        bool rv_ends_with = false;

        if( is_empty() == false )
        {
            if( m_buffer_view[length() - 1] == character )
            {
                rv_ends_with = true;
            }
        }

        return rv_ends_with;
    }

    constexpr bool String_view::ends_with( const char* p_C_string ) const
    {
        bool rv_ends_with = true;
        size_t suffix_length = 0;

        // Calculate the length of the C-string
        while( p_C_string[suffix_length] != '\0' )
        {
            ++suffix_length;
        }

        if( suffix_length <= length() )
        // suffix can not be longer than this string
        {
            size_t start_pos = length() - suffix_length;

            for( size_t i = 0; i < suffix_length; ++i )
            {
                if( p_C_string[i] != m_buffer_view[start_pos + i] )
                {
                    rv_ends_with = false;
                    break;
                }
            }
        }
        else
        {
            rv_ends_with = false;
        }

        return rv_ends_with;
    }

    constexpr bool String_view::ends_with( String_view str ) const
    {
        bool rv_ends_with = true;

        if( str.length() <= length() )
        // str can not be longer than this string
        {
            size_t start_pos = length() - str.length();

            for( size_t i = 0; i < str.length(); ++i )
            {
                if( str.m_buffer_view[i] != m_buffer_view[start_pos + i] )
                {
                    rv_ends_with = false;
                    break;
                }
            }
        }
        else
        {
            rv_ends_with = false;
        }

        return rv_ends_with;
    }

    constexpr String_view String_view::trim( void ) const
    {
        return trim_left().trim_right();
    }

    constexpr String_view String_view::trim_left( void ) const
    {
        size_t trimmed_characters_cnt = 0;

        for( char ch: *this )
        {
            if( std::isspace( ch ) == false )
            {
                break;
            }

            ++trimmed_characters_cnt;
        }

        return String_view( m_buffer_view.sub_buffer( trimmed_characters_cnt, length() - trimmed_characters_cnt ) );
    }

    constexpr String_view String_view::trim_right( void ) const
    {
        size_t trimmed_characters_cnt = 0;

        for( char ch: Reverse_iterator( *this ) )
        {
            if( std::isspace( ch ) == false )
            {
                break;
            }

            ++trimmed_characters_cnt;
        }

        return String_view( m_buffer_view.sub_buffer( 0, length() - trimmed_characters_cnt ) );
    }

    constexpr char String_view::operator[]( size_t index ) const
    {
        return m_buffer_view[index];
    }

    constexpr bool String_view::find( String_view sub_string,
                                      size_t& position ) const
    {
        bool rv_found = false;

        if( ( sub_string.is_empty() == false ) && ( sub_string.length() <= length() ) )
        {
            for( size_t i = 0; i <= length() - sub_string.length(); ++i )
            {
                bool match = true;

                for( size_t j = 0; j < sub_string.length(); ++j )
                {
                    if( m_buffer_view[i + j] != sub_string.m_buffer_view[j] )
                    {
                        match = false;
                        break;
                    }
                }

                if( match )
                {
                    position = i;
                    rv_found = true;
                    break;
                }
            }
        }

        return rv_found;
    }

    constexpr bool String_view::find( char character,
                                      size_t& position ) const
    {
        bool rv_found = false;

        for( size_t i = 0u; i < length(); ++i )
        {
            if( m_buffer_view[i] == character )
            {
                position = i;
                rv_found = true;
                break;
            }
        }

        return rv_found;
    }

    constexpr bool String_view::find_last( char character,
                                           size_t& position ) const
    {
        bool rv_found = false;

        for( size_t i = length(); i > 0u; --i )
        {
            if( m_buffer_view[i - 1] == character )
            {
                position = i - 1;
                rv_found = true;
                break;
            }
        }

        return rv_found;
    }

    constexpr String_view String_view::sub_string( size_t start_idx,
                                                   size_t cnt ) const
    {
        return String_view( m_buffer_view.sub_buffer( start_idx, cnt ) );
    }

    constexpr bool String_view::is_single_word() const
    {
        bool rv_is_single_word = true;

        for( size_t i = 0u; i < length(); ++i )
        {
            if( std::isspace( m_buffer_view[i] ) != 0 )
            {
                rv_is_single_word = false;
                break;
            }
        }

        return rv_is_single_word;
    }

    constexpr Buffer_view<char> String_view::buffer( void ) const
    {
        return m_buffer_view;
    }

    constexpr size_t String_view::detect_base_and_strip_prefix( String_view& out_number_str,
                                                                size_t expected_base ) const
    {
        size_t detected_base = 10;

        if( out_number_str.length() >= 2 )
        {
            char first_char = out_number_str[0];

            if( first_char == '0' )
            {
                char second_char = out_number_str[1];

                if( ( second_char == 'x' ) || ( second_char == 'X' ) )
                {
                    detected_base = 16;

                    if( ( expected_base == 0 ) || ( expected_base == 16 ) )
                    // strip the prefix
                    {
                        out_number_str = out_number_str.sub_string( 2 );
                    }
                }
                else if( ( second_char == 'b' ) || ( second_char == 'B' ) )
                {
                    detected_base = 2;

                    if( ( expected_base == 0 ) || ( expected_base == 2 ) )
                    // strip the prefix
                    {
                        out_number_str = out_number_str.sub_string( 2 );
                    }
                }
                else if( ( second_char >= '1' ) && ( second_char <= '7' ) )
                {
                    detected_base = 8;

                    if( ( expected_base == 0 ) || ( expected_base == 8 ) )
                    // strip the prefix
                    {
                        out_number_str = out_number_str.sub_string( 1 );
                    }
                }
                else
                {
                    if( ( expected_base == 0 ) || ( expected_base == 10 ) )
                    {
                        out_number_str = out_number_str.sub_string( 1 );
                    }
                }
            }
            else
            // "0"
            {
                // intentionally empty
            }
        }

        if( expected_base != 0u )
        // forcing base
        {
            detected_base = expected_base;
        }

        return detected_base;
    }

    constexpr bool String_view::char_to_digit( char ch,
                                               size_t base,
                                               uint8_t& digit_value ) const
    {
        bool rv_is_valid = true;

        // Convert character to digit value
        if( ch >= '0' && ch <= '9' )
        {
            digit_value = ch - '0';
        }
        else if( ch >= 'a' && ch <= 'z' )
        {
            digit_value = ch - 'a' + 10;
        }
        else if( ch >= 'A' && ch <= 'Z' )
        {
            digit_value = ch - 'A' + 10;
        }
        else
        {
            rv_is_valid = false;
        }

        if( rv_is_valid )
        {
            rv_is_valid = digit_value < base;
        }

        return rv_is_valid;
    }

    constexpr bool String_view::to_hex_bytes( Array<uint8_t> output ) const
    {
        bool rv_success = starts_with( "0x" );

        rv_success = rv_success && ( ( length() - 2u ) % 2u == 0 ); // must have even number of hex digits

        if( rv_success )
        {
            size_t byte_count = ( length() - 2u ) / 2u;

            rv_success = output.size() >= byte_count;

            if( rv_success )
            {
                for( size_t i = 0; i < byte_count; ++i )
                {
                    String_view byte_str = sub_string( 2u + ( i * 2u ), 2u );

                    if( byte_str.to( output[i], 16u ) == false )
                    {
                        rv_success = false;
                        break;
                    }
                }
            }
        }

        return rv_success;
    }
} // namespace SOMA

#endif // SOMA_STRING_VIEW_H_
