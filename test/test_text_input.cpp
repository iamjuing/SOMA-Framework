// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/text/text_input.h"
#include "soma/io/i_blocking_input_stream.h"
#include "soma/text/string.h"

using namespace SOMA;

class Mock_input_stream: public I_blocking_input_stream
{
public:
    Mock_input_stream( const char* p_string )
        : mp_string( p_string ),
          m_length( strlen( p_string ) )
    {
        // intentionally empty
    }

    void read( Buffer<uint8_t>& out_buffer ) override
    {
        size_t to_copy = std::min( m_length - m_position_idx,
                                   out_buffer.space() );

        // NOTE: const_cast is safe here as the String_view has only const methods
        out_buffer.insert( String_view( Buffer_view( Array( const_cast<char*>( &mp_string[m_position_idx] ), to_copy ) ) ) );

        m_position_idx += to_copy;
    }

    void read( Buffer<uint8_t>& out_buffer,
               uint32_t timeout_ms ) override
    {
        // intentionally empty
    }

private:
    const char* mp_string = nullptr;
    size_t m_length;
    size_t m_position_idx = 0;
};

TEST( text_input, substrings )
{
    Mock_input_stream mock_stream( "  text string    for     test       parsing  " );
    Text_input text_input( mock_stream );

    String::Length<40> substring;

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring, "text" );

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring, "string" );

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring, "for" );

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring, "test" );

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring, "parsing" );
}

TEST( text_input, numbers )
{
    Mock_input_stream mock_stream( "  text 12    66.66    0xBEEF   " );
    Text_input text_input( mock_stream );

    String::Length<40> substring;
    uint32_t integer_number;
    float floating_point_number;

    EXPECT_FALSE( text_input >> integer_number );

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring, "text" );

    EXPECT_TRUE( text_input >> integer_number );
    EXPECT_EQ( integer_number, 12 );

    EXPECT_FALSE( text_input >> integer_number );

    EXPECT_TRUE( text_input >> floating_point_number );
    EXPECT_EQ( floating_point_number, 66.6600031f );

    EXPECT_TRUE( text_input >> floating_point_number );
    EXPECT_EQ( floating_point_number, 0xBEEF );
}

TEST( text_input, parse_multiple_values )
{
    Mock_input_stream mock_stream( "  text 12    66.66 " );
    Text_input text_input( mock_stream );

    String::Length<40> substring;
    uint32_t integer_number;
    float floating_point_number;

    EXPECT_TRUE( text_input >> substring >> integer_number >> floating_point_number );

    EXPECT_EQ( substring.view(), "text" );
    EXPECT_EQ( integer_number, 12 );
    EXPECT_EQ( floating_point_number, 66.6600031f );
}

TEST( text_input, single_character )
{
    Mock_input_stream mock_stream( "? " );
    Text_input text_input( mock_stream );

    String::Length<40> substring;

    EXPECT_TRUE( text_input >> substring );
    EXPECT_EQ( substring.view(), "?" );
}
