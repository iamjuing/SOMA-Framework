// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_STRING_TABLE_GENRATOR_H_
#define SOMA_STRING_TABLE_GENRATOR_H_

#include "soma/text/text_output.h"
#include "soma/containers/buffer.h"

namespace SOMA
{
    class String_view;
    class Text_output;

    class String_table_generator
    {
    public:
        enum class Cell_alignment_e: uint8_t
        {
            left,
            right,
            center
        };

        struct Column_t
        {
            String_view m_name;
            uint8_t m_width;
            Cell_alignment_e m_alignment;
        };

        struct Layout_t
        {
            String_view m_column_separator;
            char m_header_separator;
            const Array<const Column_t> columns;
        };

        String_table_generator( const Layout_t& layout,
                                Text_output& outStream );

        String_table_generator& operator<<( String_view str );
        String_table_generator& operator<<( const char* c_str );

        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        String_table_generator& operator<<( T number );

        void header();

    private:
        const Layout_t m_layout;
        Text_output& m_output_stream;

        uint8_t m_current_column = 0;
    };

    template<typename T, typename>
    String_table_generator& String_table_generator::operator<<( T number )
    {
        String::Length<30> number_str;
        number_str += number;

        return ( *this ) << number_str.view();
    }
} // namespace SOMA

#endif // STRING_TABLE_GENRATOR_H_
