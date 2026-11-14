// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_STRING_INPUT_H_
#define SOMA_STRING_INPUT_H_

#include "soma/text/text_input.h"
#include "soma/io/i_blocking_input_stream.h"
#include "soma/time.h"

namespace SOMA
{
    class String_view;

    ///
    /// @brief Converts String_view to Text_input stream for parsing.
    ///
    ///        Usage:
    ///              String_view str = "distance: 12 speed: 3.5"
    ///              Text_string_input string_input( str );
    ///
    ///              float value;
    ///              String_view key;
    ///
    ///              stdin >> key;
    ///              stdin >> value;
    ///              DLOG("%*s %ld", key.length(), key, value);
    ///
    ///              stdin >> key;
    ///              stdin >> value;
    ///              DLOG("%*s %ld", key.length(), key, value);
    ///
    ///        Usage:
    ///              String_view str = "text 42 66.66"
    ///              Text_string_input string_input( str );
    ///
    ///              String_view substring;
    ///              float value1;
    ///              uint32_t value2;
    ///
    ///              if (string_input >> substring >> value1 >> value2)
    ///              {
    ///                  DLOG("%*s %f %ld", substring.length(), substring, value, value1, value2);
    ///              }
    class Text_string_input: public Text_input
    {
    public:
        explicit Text_string_input( const String_view& string );

        bool is_empty( void ) override;

        String_view get_line( void );
        String_view get_line( char delimiter );

        Text_string_input& operator>>( String_view& out_string );

        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        Text_string_input& operator>>( T& out_number );

    private:

        class Stream: public I_blocking_input_stream
        {
        public:
            explicit Stream( const String_view& input_string );

            ~Stream() override = default;

            void read( Buffer<uint8_t>& out_buffer ) override;

            void read( Buffer<uint8_t>& out_buffer,
                       uint32_t timeout_ms ) override;

        private:
            String_view m_input_string;
            size_t m_position_idx = 0;

            friend class Text_string_input;
        };

        Stream m_input_string_stream;

        void return_back( String_view string ) override;
        void return_back( char ch ) override;
    };

    template<typename T, typename>
    Text_string_input& Text_string_input::operator>>( T& out_number )
    {
        if( m_is_error_active == false )
        {
            Text_input::operator>>( out_number );
        }

        return *this;
    }
} // namespace SOMA

#endif // STRING_INPUT_H_
