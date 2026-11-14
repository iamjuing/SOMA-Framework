// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_STRING_H_
#define SOMA_STRING_H_

#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#include "soma/containers/buffer.h"
#include "soma/internal/compiler_attributes.h"
#include "soma/text/string_view.h"
#include "soma/io/i_blocking_output_stream.h"

namespace SOMA
{
    ///
    /// @brief Mutable, null-terminated string with fixed capacity.
    ///
    /// String uses caller-owned storage. String::Length<N> owns storage for N
    /// characters plus the null terminator. Both can be passed to a function
    /// accepting String&. External storage must outlive the string.
    ///
    /// @code
    /// using SOMA::Buffer;
    /// using SOMA::String;
    /// void string_examples()
    /// {
    ///     char external[9];
    ///     String view { Buffer<char>{ external } }; // Capacity: 8 characters.
    ///     String::Length<8> owned { "text" };       // Owns nine bytes.
    ///     String& common = owned;
    ///     common += '!';
    ///     view = common.view();
    /// }
    /// @endcode
    namespace Internal
    {
        template<size_t N>
        class String_owned;
    }

    class String
    {
    public:
        template<size_t N>
        using Length = Internal::String_owned<N>;

        explicit String( Buffer<char> buffer );
        String( const String& text ) = delete;
        String& operator=( const String& ) = delete;

        String_view view( void ) const;
        operator String_view() const;

        char& operator[]( size_t index );
        char operator[]( size_t index ) const;

        void clear( void );

        size_t capacity( void ) const;
        bool is_empty( void ) const;
        bool is_full( void ) const;

        size_t length( void ) const;
        size_t space( void ) const;

        size_t format( const char* p_format ... ) SOMA_COMPILER_PRINTF_FORMAT( 2, 3 );
        size_t vformat( const char* p_format,
                        va_list args );

        void fraction( int32_t numerator,
                       int32_t denominator,
                       const char* p_format,
                       size_t decimal_digit_cnt );
        void fraction( uint32_t numerator,
                       uint32_t denominator,
                       const char* p_format,
                       size_t decimal_digit_cnt );

        bool operator==( const char* rhs ) const;
        bool operator==( const String_view& rhs ) const;
        bool operator==( const String& rhs ) const;

        String& operator=( const char* p_C_string );
        String& operator=( const String_view& str );

        void operator+=( bool value );
        void operator+=( char c );
        void operator+=( float number );
        void operator+=( double number );

        template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_signed<T>>, int> = 0>
        void operator+=( T number );

        template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>, int> = 0>
        void operator+=( T number );

        void operator+=( const char* p_C_string );
        void operator+=( const String_view& rhs );

        void operator-=( size_t count );

        void trim_right( void );

        const char* C_string( void ) const;

    private:
        Buffer<char> m_buffer;

        template<typename T>
        void append_unsigned_decimal( T number );

        // Forbid implicit conversion to (const) char*, to avoid accidentally comparing pointers
        // instead of text when using operator==().
        // NOTE: Must be declared 'explicit' as the compiler tries to use it even though it is marked deleted
        //       and generates a warning (ambiguous conversion).
        explicit operator const char*() const = delete;
        explicit operator char*() const = delete;
    };

    template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_signed<T>>, int>>
    void String::operator+=( T number )
    {
        using Unsigned_t = std::make_unsigned_t<T>;

        auto magnitude = static_cast<Unsigned_t>( number );

        if( number < 0 )
        {
            operator+=( '-' );
            magnitude = static_cast<Unsigned_t>( 0u ) - magnitude;
        }

        append_unsigned_decimal( magnitude );
    }

    template<typename T, std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>, int>>
    void String::operator+=( T number )
    {
        append_unsigned_decimal( number );
    }

    template<typename T>
    void String::append_unsigned_decimal( T number )
    {
        static_assert( std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>, "Unsigned integral type expected!" );
        static_assert( sizeof( T ) <= sizeof( uint64_t ), "Unsupported type!" );

        char digits[std::numeric_limits<uint64_t>::digits10 + 1u];
        size_t digit_count = 0u;

        do
        {
            digits[digit_count] = static_cast<char>( '0' + ( number % 10u ) );
            number /= 10u;
            ++digit_count;
        } while( number > 0u );

        while( digit_count > 0u )
        {
            --digit_count;
            operator+=( digits[digit_count] );
        }
    }

    namespace Internal
    {
        template<size_t N>
        struct String_storage_t
        {
            String_storage_t()
            {
                // intentionally empty
            }

            char m_storage[N + 1u];
        };

        template<size_t N>
        class String_owned: private String_storage_t<N>, public String
        {
        public:
            String_owned( void );
            explicit String_owned( const char* p_C_string );
            String_owned( const String_owned& ) = delete;
            String_owned( String_owned&& ) = delete;
            String_owned& operator=( const String_owned& ) = delete;
            String_owned& operator=( String_owned&& ) = delete;

            using String::operator=;

        private:
            using Storage = String_storage_t<N>;
        };

        template<size_t N>
        String_owned<N>::String_owned( void )
            : Storage(),
              String( Buffer<char>( Array<char>( Storage::m_storage, N + 1u ) ) )
        {
        }

        template<size_t N>
        String_owned<N>::String_owned( const char* p_C_string )
            : String_owned()
        {
            String::operator=( p_C_string );
        }
    }
} // namespace SOMA

#endif // SOMA_STRING_H_
