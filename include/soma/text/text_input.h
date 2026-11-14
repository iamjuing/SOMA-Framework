// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_INPUT_STRING_STREAM_H_
#define SOMA_INPUT_STRING_STREAM_H_

#include <type_traits>

#include "soma/text/string.h"

namespace SOMA
{
    class String_view;
    class I_blocking_input_stream;

    ///
    /// @brief Represents textual input that is read from provided blocking input stream. The text can be parsed
    ///        using opearator>> to various types.
    ///
    ///        Usage: Transform (non-blocking) input stream to a blocking input stream, and use it with Text_input.
    ///
    ///              Blocking_input_stream blocking_UART_input_stream { UART_input_stream };
    ///              Text_input stdin { UART_input_stream };
    ///              int32_t value;
    ///
    ///              if (stdin >> value)
    ///              {
    ///                  DLOG("Value: %ld", value);
    ///              }
    ///              else
    ///              {
    ///                  ELOG("Integer number expected!");
    ///              }
    class Text_input
    {
    public:
        explicit Text_input( I_blocking_input_stream& input_stream );
        virtual ~Text_input() = default;

        operator bool() const;

        virtual bool is_empty( void );
        I_blocking_input_stream& blocking_stream( void );

        Text_input& operator>>( String& out_text );

        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        Text_input& operator>>( T& out_number );

    protected:
        bool read( char& out_char );

        virtual void return_back( String_view string );
        virtual void return_back( char ch );

        mutable bool m_is_error_active = false;

    private:
        static const size_t mc_number_buffer_size = 30; // arithmetic values shall fit this buffer
        String::Length<mc_number_buffer_size + 1> m_returned_str; // +1 for returned character
        size_t m_number_string_position = 0;

        I_blocking_input_stream& m_input_stream;
    };

    template<typename T, typename>
    Text_input& Text_input::operator>>( T& out_number )
    {
        if( m_is_error_active == false )
        {
            bool succeeded = false;
            String::Length<mc_number_buffer_size> m_number_str;
            bool character_read = false;
            bool has_decimal_point = false;
            char ch;

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
                if( ch == '.' )
                {
                    has_decimal_point = true;
                }

                m_number_str += ch;

                while( read( ch ) )
                {
                    if( std::isspace( ch ) || m_number_str.is_full() )
                    {
                        break;
                    }
                    else
                    {
                        if( ch == '.' )
                        {
                            has_decimal_point = true;
                        }

                        m_number_str += ch;
                    }
                }

                if( ( std::is_integral_v<T> && ( has_decimal_point == false ) ) || std::is_floating_point_v<T> )
                {
                    if( m_number_str.view().to( out_number ) )
                    {
                        succeeded = true;
                    }
                }
            }

            if( succeeded == false )
            // return everything back
            // NOTE: The trimmed whitespaces are lost!
            {
                m_number_str += ch;
                return_back( m_number_str.view() );

                m_is_error_active = true;
            }
        }

        return *this;
    }
} // namespace SOMA

#endif // INPUT_STRING_STREAM_H_
