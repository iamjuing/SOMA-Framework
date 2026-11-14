// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/text/string_table_generator.h"
#include "soma/text/string_view.h"
#include "soma/text/text_output.h"
#include "soma/text/text_string_input.h"

namespace SOMA
{
    String_table_generator::String_table_generator( const Layout_t& layout,
                                                    Text_output& outStream )
        : m_layout( layout ),
          m_output_stream( outStream )
    {
        // intentionally empty
    }

    String_table_generator& String_table_generator::operator<<( String_view str )
    {
        const auto& col = m_layout.columns[m_current_column]; // current column info
        size_t column_width = col.m_width;
        Cell_alignment_e align = col.m_alignment;

        size_t max_string_len = std::min( str.length(), column_width );

        size_t pad_left = 0;
        size_t pad_right = 0;

        switch( align )
        {
            case Cell_alignment_e::left:
                pad_left = 0;
                pad_right = column_width - max_string_len;
                break;

            case Cell_alignment_e::right:
                pad_left = column_width - max_string_len;
                pad_right = 0;
                break;

            case Cell_alignment_e::center:
                pad_left = ( column_width - max_string_len ) / 2;
                pad_right = column_width - max_string_len - pad_left;
                break;

            default:
                ASSERT( false );
                break;
        }

        // left separator
        if( m_current_column == 0u )
        {
            m_output_stream << m_layout.m_column_separator;
        }

        // left padding
        for( size_t i = 0; i < pad_left; ++i )
        {
            m_output_stream << ' ';
        }

        // cell content
        m_output_stream << str.sub_string( 0, max_string_len );

        // right padding
        for( size_t i = 0; i < pad_right; ++i )
        {
            m_output_stream << ' ';
        }

        ++m_current_column;

        if( m_current_column < m_layout.columns.size() )
        {
            m_output_stream << m_layout.m_column_separator;
        }
        else
        {
            // right separator
            m_output_stream << m_layout.m_column_separator;

            // line end
            m_output_stream << EOL;

            m_current_column = 0u;
        }

        return *this;
    }

    String_table_generator& String_table_generator::operator<<( const char* c_str )
    {
        return ( *this ) << String_view( c_str );
    }

    void String_table_generator::header()
    {
        String_view reminder_str[m_layout.columns.size()];
        bool finished = false;

        // intialize string reminders
        for( uint8_t i = 0; i < m_layout.columns.size(); ++i )
        {
            reminder_str[i] = m_layout.columns[i].m_name.trim_left().trim_right();
        }

        do
        // rows
        {
            finished = true;

            // left separator
            m_output_stream << m_layout.m_column_separator;

            for( uint8_t i = 0; i < m_layout.columns.size(); ++i )
            {
                const auto& column = m_layout.columns[i];

                if( reminder_str[i].is_empty() == false )
                {
                    String_view row_str;

                    if( reminder_str[i].length() > column.m_width )
                    // split the string
                    {
                        Text_string_input input( reminder_str[i] );

                        input >> row_str;
                    }
                    else
                    {
                        row_str = reminder_str[i];
                    }

                    if( row_str.length() <= column.m_width )
                    // shorter, padding needed
                    {
                        size_t pad_left = ( column.m_width - row_str.length() ) / 2;
                        size_t pad_right = column.m_width - row_str.length() - pad_left;

                        // left padding
                        for( size_t j = 0; j < pad_left; ++j )
                        {
                            m_output_stream << ' ';
                        }

                        m_output_stream << row_str;

                        // right padding
                        for( size_t j = 0; j < pad_right; ++j )
                        {
                            m_output_stream << ' ';
                        }
                    }
                    else if( row_str.length() == column.m_width )
                    // fits perfectly
                    {
                        m_output_stream << row_str;
                    }
                    else
                    // truncate
                    {
                        // string is longer than column width,
                        m_output_stream << row_str.sub_string( 0, column.m_width );
                    }

                    // next row
                    reminder_str[i] = reminder_str[i].sub_string( row_str.length() ).trim_left();

                    if( reminder_str[i].is_empty() == false )
                    {
                        finished = false;
                    }
                }
                else
                {
                    // empty column, fill with spaces
                    for( size_t j = 0; j < column.m_width; ++j )
                    {
                        m_output_stream << ' ';
                    }
                }

                m_output_stream << m_layout.m_column_separator;
            }

            m_output_stream << EOL;
        } while( finished == false );

        // under the left separator
        for( size_t i = 0; i < m_layout.m_column_separator.length(); ++i )
        {
            m_output_stream << m_layout.m_header_separator;
        }

        // header separator
        for( uint8_t i = 0; i < m_layout.columns.size(); ++i )
        {
            for( size_t j = 0; j < m_layout.columns[i].m_width; ++j )
            {
                m_output_stream << m_layout.m_header_separator;
            }

            // under the right separator
            for( size_t j = 0; j < m_layout.m_column_separator.length(); ++j )
            {
                m_output_stream << m_layout.m_header_separator;
            }
        }

        m_output_stream << EOL;
    }
} // namespace SOMA
