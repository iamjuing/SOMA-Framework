// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TEXT_OUTPUT_H_
#define SOMA_TEXT_OUTPUT_H_

#include <type_traits>
#include <cstdint>
#include <cinttypes>

#include "soma/text/string.h"
#include "soma/containers/buffer.h"
#include "soma/array.h"

namespace SOMA
{
    class String_view;
    class B_variable;
    class I_blocking_output_stream;
    class min_width;
    class fill_with;
    class precision;

    // Forward declare template classes
    template<typename Numerator_t, typename Denominator_t>
    class fraction;

    template<typename T>
    class memory_dump;

    template<typename T>
    class bytes;

    ///
    /// @brief Represents a universal formatted textual output that is pushed to the provided stream. String is pushed to the stream
    ///        using opearator<< and provides public members for formatting purposes (color, new line etc.)
    ///        The typical usage is to redirect the formatted text to a console (UART/TCP) port.
    ///
    ///        Usage:
    ///              Transform output stream to a blocking output stream, and use it with Text_output.
    ///
    ///              Blocking_output_stream blocking_UART_output_stream { UART_output_stream };
    ///              Text_output output { blocking_UART_output_stream };
    ///
    ///              output << "Value: " << 3.14f << EOL;
    ///
    class Text_output
    {
    public:
        explicit Text_output( I_blocking_output_stream& output_stream );
        explicit Text_output( I_blocking_output_stream& output_stream,
                              Buffer<char>& line_buffer );

        ~Text_output();

        void flush( void );
        I_blocking_output_stream& blocking_stream( void );
        uint8_t precision( void ) const;

        Text_output& operator<<( const B_variable& P );
        Text_output& operator<<( const char* p_str );
        Text_output& operator<<( const String_view& str );
        Text_output& operator<<( bool value );
        Text_output& operator<<( Text_output& ( *manipulator ) ( Text_output& ) ); // for zero-parameter free form manipulators
        Text_output& operator<<( const min_width& min_width );
        Text_output& operator<<( const fill_with& fill_with );
        Text_output& operator<<( const class precision& precision );

        template<typename T>
        Text_output& operator<<( const bytes<T>& byte_count );

        template<typename T>
        Text_output& operator<<( const memory_dump<T>& dump );

        template<typename Numerator_t, typename Denominator_t>
        Text_output& operator<<( const fraction<Numerator_t, Denominator_t>& fraction );

        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        Text_output& operator<<( T number );

    private:
        static const size_t mc_number_buffer_size = 30; // arithmetic values shall fit this buffer
        static const size_t mc_default_float_precision = 6; // default in std::precision

        I_blocking_output_stream& m_output_stream;
        Buffer<char>* mp_line_buffer;

        // manipulators
        uint8_t m_min_width = 0;
        uint8_t m_precision = mc_default_float_precision; // default precision for floating-point
        bool m_align_left = false;
        bool m_print_hex = false;
        char m_pad_character = ' ';

        Text_output& write( String_view str );
        Text_output& write( char ch );

        void send( String_view str );
        void flush_line_buffer( void );

        void pad( uint8_t cnt,
                  char ch );

        // friend manipulators
        friend Text_output& left( Text_output& out );
        friend Text_output& hex( Text_output& out );
        friend class min_width;
        friend class fill_with;
        friend class precision;

        template<typename T>
        friend class memory_dump;
    };

    template<typename T, typename>
    Text_output& Text_output::operator<<( T number )
    {
        String::Length<mc_number_buffer_size> number_str;

        if( m_print_hex && std::is_integral_v<T> )
        {
            // integer variadic arguments promote to int or a wider type
            if constexpr( sizeof( T ) <= sizeof( int ) )
            {
                number_str.format( "%X", static_cast<unsigned int>( number ) );
            }
            else if constexpr( sizeof( T ) == sizeof( long ) )
            {
                number_str.format( "%lX", static_cast<unsigned long>( number ) );
            }
            else if constexpr( sizeof( T ) == sizeof( long long ) )
            {
                number_str.format( "%llX", number );
            }
            else
            {
                static_assert( !sizeof( T ), "Unsupported type!" );
            }
        }
        else if constexpr( std::is_floating_point_v<T> )
        {
            number_str.format( "%.*f", m_precision, number );
        }
        else
        {
            number_str += number;
        }

        return write( number_str );
    }

    // text output manipulator class
    class min_width
    {
    public:
        explicit constexpr min_width( uint8_t min_width )
            : mc_min_width( min_width )
        {
            // intentionally empty
        }

    private:
        const uint8_t mc_min_width;

        friend class Text_output;
    };

    // text output manipulator class
    class fill_with
    {
    public:
        explicit constexpr fill_with( char fill_with )
            : mc_fill_with( fill_with )
        {
            // intentionally empty
        }

    private:
        const char mc_fill_with;

        friend class Text_output;
    };

    // text output manipulator class
    class precision
    {
    public:
        explicit constexpr precision( uint8_t precision )
            : mc_precision( precision )
        {
            // intentionally empty
        }

    private:
        const uint8_t mc_precision;

        friend class Text_output;
    };

    // text output manipulator class for displaying bytes in human-readable format
    template<typename T>
    class bytes
    {
    private:
        using Value_t = std::remove_cv_t<T>;

    public:
        explicit constexpr bytes( T bytes,
                                  size_t decimal_digits )
            : mc_bytes( bytes ),
              mc_decimal_digits( decimal_digits )
        {
            static_assert( std::is_integral_v<Value_t>, "Bytes type must be integral" );
            static_assert( std::is_unsigned_v<Value_t>, "Bytes type must be unsigned" );
            static_assert( !std::is_same_v<Value_t, bool>, "Bytes type must not be bool" );
            static_assert( sizeof( Value_t ) <= sizeof( uint64_t ), "Bytes type must be at most 64 bits" );
        }

    private:
        const Value_t mc_bytes;
        const size_t mc_decimal_digits;

        friend class Text_output;
    };

    ///
    /// @brief Text output manipulator class for displaying Array<> in memory dump format.
    ///        Formats numeric arrays with an index column followed by values in configurable columns.
    ///
    /// @tparam T The array element type (must be arithmetic/numeric)
    ///
    /// Usage example:
    ///     Array<uint8_t, 16> data = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    ///                                        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    ///
    ///     // Basic memory dump with 8 columns per row
    ///     output << memory_dump( data, 8 ) << EOL;
    ///
    ///     // Memory dump in hexadecimal with minimum width and padding
    ///     output << hex << min_width( 2 ) << fill_with( '0' )
    ///            << memory_dump( data, 8 ) << EOL;
    ///
    ///     // Memory dump with custom starting index (useful for partial dumps)
    ///     output << hex << min_width( 2 ) << fill_with( '0' )
    ///            << memory_dump( data, 8, 0x1000 ) << EOL;
    ///
    ///     // Output:
    ///     //    0: 00 11 22 33 44 55 66 77
    ///     //    8: 88 99 AA BB CC DD EE FF
    ///     // 1000: 00 11 22 33 44 55 66 77
    ///     // 1008: 88 99 AA BB CC DD EE FF
    template<typename T>
    class memory_dump
    {
    public:
        explicit constexpr memory_dump( Array<const T> array,
                                        size_t columns_per_row,
                                        size_t initial_index = 0u )
            : mc_array( array ),
              mc_columns_per_row( columns_per_row ),
              mc_initial_index( initial_index )
        {
            static_assert( std::is_arithmetic_v<T>, "Array element type must be numeric" );
        }

    private:
        Array<const T> mc_array;
        const size_t mc_columns_per_row;
        const size_t mc_initial_index;

        friend class Text_output;
    };

    template<typename T>
    memory_dump( Array<T>, size_t, size_t = 0u ) -> memory_dump<std::remove_const_t<T>>;

    // text output manipulator class for integer fractions
    template<typename Numerator_t, typename Denominator_t>
    class fraction
    {
    public:
        // Constructor for signed types
        template<typename N = Numerator_t, typename D = Denominator_t,
                 typename = std::enable_if_t<std::is_signed_v<N> && std::is_signed_v<D>>>
        constexpr fraction( Numerator_t numerator,
                            Denominator_t denominator,
                            size_t decimal_digits )
            : mc_numerator( numerator ),
              mc_denominator( denominator ),
              mc_decimal_digits( decimal_digits )
        {
            static_assert( std::is_integral_v<Numerator_t>, "Numerator must be an integral type" );
            static_assert( std::is_integral_v<Denominator_t>, "Denominator must be an integral type" );
        }

        // Constructor for unsigned types
        template<typename N = Numerator_t, typename D = Denominator_t,
                 typename = std::enable_if_t<std::is_unsigned_v<N> && std::is_unsigned_v<D>>,
                 typename = void>
        constexpr fraction( Numerator_t numerator,
                            Denominator_t denominator,
                            size_t decimal_digits )
            : mc_numerator( numerator ),
              mc_denominator( denominator ),
              mc_decimal_digits( decimal_digits )
        {
            static_assert( std::is_integral_v<Numerator_t>, "Numerator must be an integral type" );
            static_assert( std::is_integral_v<Denominator_t>, "Denominator must be an integral type" );
        }

    private:
        const Numerator_t mc_numerator;
        const Denominator_t mc_denominator;
        const size_t mc_decimal_digits;

        template<typename N, typename D>
        friend Text_output& Text_output::operator<<( const fraction<N, D>& fraction );
    };

    template<typename Numerator_t, typename Denominator_t>
    Text_output& Text_output::operator<<( const fraction<Numerator_t, Denominator_t>& fraction )
    {
        // Calculate integer and remainder parts
        auto integer = fraction.mc_numerator / fraction.mc_denominator;
        auto remainder = fraction.mc_numerator % fraction.mc_denominator;
        auto denominator = fraction.mc_denominator;

        // For signed types, make remainder and denominator positive
        if constexpr( std::is_signed_v<Numerator_t> )
        {
            if( remainder < 0 )
            {
                remainder = ( ~remainder ) + 1;
            }

            if( denominator < 0 )
            {
                denominator = ( ~denominator ) + 1;
            }
        }

        // Calculate decimal shift (10^decimal_digits)
        uint32_t decimal_shift = 1;

        for( size_t i = 0; i < fraction.mc_decimal_digits; ++i )
        {
            decimal_shift *= 10;
        }

        // Calculate decimal part with rounding
        uint64_t decimal = static_cast<uint64_t>( remainder ) * decimal_shift;

        decimal += static_cast<uint64_t>( denominator ) >> 1; // rounding away from 0
        decimal /= static_cast<uint64_t>( denominator );

        // Handle rounding overflow (e.g., 0.999... -> 1.000)
        if( decimal == decimal_shift )
        {
            if constexpr( std::is_signed_v<Numerator_t> )
            {
                ( integer < 0 ) ? ( integer -= 1 ) : ( integer += 1 );
            }
            else
            {
                integer += 1;
            }
            decimal = 0;
        }

        // Output integer part
        *this << integer;

        // Output decimal part if needed
        if( fraction.mc_decimal_digits > 0u )
        {
            *this << '.';

            auto decimal_value = static_cast<uint32_t>( decimal );

            // Calculate the number of digits in the decimal value
            size_t actual_digits = 0;

            if( decimal_value == 0 )
            {
                actual_digits = 1;
            }
            else
            {
                uint32_t temp = decimal_value;

                while( temp > 0 )
                {
                    temp /= 10;
                    actual_digits++;
                }
            }

            // Add leading zeros if needed
            if( actual_digits < fraction.mc_decimal_digits )
            {
                for( size_t i = 0; i < ( fraction.mc_decimal_digits - actual_digits ); ++i )
                {
                    *this << '0';
                }
            }

            *this << decimal_value;
        }

        return *this;
    }

    template<typename T>
    Text_output& Text_output::operator<<( const bytes<T>& byte_count )
    {
        static constexpr uint64_t mc_bytes_per_kB = 1024u;
        static constexpr uint64_t mc_bytes_per_MB = mc_bytes_per_kB * mc_bytes_per_kB;

        if( byte_count.mc_bytes == 0u )
        {
            *this << 0
                  << " B";
        }
        else if( byte_count.mc_bytes < mc_bytes_per_kB )
        {
            *this << byte_count.mc_bytes
                  << " B";
        }
        else if( byte_count.mc_bytes < mc_bytes_per_MB )
        {
            *this << fraction( byte_count.mc_bytes, mc_bytes_per_kB, byte_count.mc_decimal_digits )
                  << " kB";
        }
        else
        {
            *this << fraction( byte_count.mc_bytes, mc_bytes_per_MB, byte_count.mc_decimal_digits )
                  << " MB";
        }

        return *this;
    }

    template<typename T>
    Text_output& Text_output::operator<<( const memory_dump<T>& dump )
    {
        ASSERT( dump.mc_columns_per_row > 0u );

        const auto& array = dump.mc_array;
        const size_t columns = dump.mc_columns_per_row;

        // Save current formatting state
        bool original_hex = m_print_hex;
        uint8_t original_min_width = m_min_width;
        char original_pad_char = m_pad_character;

        // Calculate maximum width needed for values
        uint8_t value_width = m_min_width;

        if( value_width == 0u )
        {
            // Auto-calculate width based on type and format
            if( m_print_hex )
            {
                constexpr uint8_t c_hex_digits_per_byte = 2;

                value_width = sizeof( T ) * c_hex_digits_per_byte;
            }
            else
            {
                // Calculate width for decimal representation
                if constexpr( std::is_signed_v<T> )
                {
                    // For signed types, find max negative value width
                    constexpr uint8_t c_width_int8 = 4; // -128
                    constexpr uint8_t c_width_int16 = 6; // -32768
                    constexpr uint8_t c_width_int32 = 11; // -2147483648
                    constexpr uint8_t c_width_int64 = 20; // -9223372036854775808

                    if constexpr( sizeof( T ) == 1 )
                    {
                        value_width = c_width_int8;
                    }
                    else if constexpr( sizeof( T ) == 2 )
                    {
                        value_width = c_width_int16;
                    }
                    else if constexpr( sizeof( T ) == 4 )
                    {
                        value_width = c_width_int32;
                    }
                    else if constexpr( sizeof( T ) == 8 )
                    {
                        value_width = c_width_int64;
                    }
                }
                else
                {
                    // For unsigned types
                    constexpr uint8_t c_width_uint8 = 3; // 255
                    constexpr uint8_t c_width_uint16 = 5; // 65535
                    constexpr uint8_t c_width_uint32 = 10; // 4294967295
                    constexpr uint8_t c_width_uint64 = 20; // 18446744073709551615

                    if constexpr( sizeof( T ) == 1 )
                    {
                        value_width = c_width_uint8;
                    }
                    else if constexpr( sizeof( T ) == 2 )
                    {
                        value_width = c_width_uint16;
                    }
                    else if constexpr( sizeof( T ) == 4 )
                    {
                        value_width = c_width_uint32;
                    }
                    else if constexpr( sizeof( T ) == 8 )
                    {
                        value_width = c_width_uint64;
                    }
                }
            }
        }

        // Index width is fixed at 10 digits
        constexpr uint8_t c_index_width = 10;

        // Format and output each row
        for( size_t row_start = 0; row_start < array.size(); row_start += columns )
        {
            // Output index with proper width and padding (restore formatting since write() resets it)
            if( original_hex )
            {
                *this << "0x";
            }

            m_min_width = c_index_width;
            m_print_hex = original_hex;
            m_pad_character = original_pad_char;
            *this << ( dump.mc_initial_index + row_start );
            m_min_width = original_min_width; // Reset

            *this << ": ";

            // Output values in this row
            size_t row_end = ( row_start + columns < array.size() ) ? ( row_start + columns ) : array.size();

            for( size_t i = row_start; i < row_end; ++i )
            {
                if( i > row_start )
                {
                    *this << ' '; // Separator between columns
                }

                // Restore formatting state for this value (write() resets them)
                m_min_width = value_width;
                m_print_hex = original_hex;
                m_pad_character = original_pad_char;
                *this << array[i];
            }

            *this << "\r\n";
        }

        // Restore original formatting state
        m_print_hex = original_hex;
        m_min_width = original_min_width;
        m_pad_character = original_pad_char;

        return *this;
    }

    // zero-parameter text output manipulators
    Text_output& left( Text_output& output );
    Text_output& hex( Text_output& output );

    // color manipulators
    namespace Color
    {
        static constexpr String_view standard { "\033[0m" };

        static constexpr String_view black { "\033[30m" };
        static constexpr String_view red { "\033[31m" };
        static constexpr String_view green { "\033[32m" };
        static constexpr String_view yellow { "\033[33m" };
        static constexpr String_view blue { "\033[34m" };
        static constexpr String_view magenta { "\033[35m" };
        static constexpr String_view cyan { "\033[36m" };
        static constexpr String_view white { "\033[37m" };

        static constexpr String_view bright_black { "\033[90m" };
        static constexpr String_view bright_red { "\033[91m" };
        static constexpr String_view bright_green { "\033[92m" };
        static constexpr String_view bright_yellow { "\033[93m" };
        static constexpr String_view bright_blue { "\033[94m" };
        static constexpr String_view bright_magenta { "\033[95m" };
        static constexpr String_view bright_cyan { "\033[96m" };
        static constexpr String_view bright_white { "\033[97m" };
    };

    constexpr String_view EOL { "\r\n" }; // end line
} // namespace SOMA

#endif // TEXT_OUTPUT_H_
