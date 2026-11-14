// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/text/text_string_input.h"

using namespace SOMA;

TEST( text_string_input, substrings )
{
    String_view str( "  text string    for     test       parsing  " );
    Text_string_input text_string_input( str );

    String_view substring;

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "text" );

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "string" );

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "for" );

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "test" );

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "parsing" );

    EXPECT_FALSE( text_string_input >> substring );
}

TEST( Text_string_input, numbers )
{
    String_view str( "  text 12    66.66    0xBEEF   " );
    Text_string_input text_string_input( str );

    String_view substring;
    uint32_t integer_number;
    float floating_point_number;

    EXPECT_FALSE( text_string_input >> integer_number );

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "text" );

    EXPECT_TRUE( text_string_input >> integer_number );
    EXPECT_EQ( integer_number, 12 );

    EXPECT_FALSE( text_string_input >> integer_number );

    EXPECT_TRUE( text_string_input >> floating_point_number );
    EXPECT_EQ( floating_point_number, 66.6600031f );

    EXPECT_TRUE( text_string_input >> floating_point_number );
    EXPECT_EQ( floating_point_number, 0xBEEF );
}

TEST( text_string_input, parse_multiple_values )
{
    String_view str( "  text 12    66.66   " );
    Text_string_input text_string_input( str );

    String_view substring;
    uint32_t integer_number;
    float floating_point_number;

    EXPECT_TRUE( text_string_input >> substring >> integer_number >> floating_point_number );

    EXPECT_EQ( substring, "text" );
    EXPECT_EQ( integer_number, 12 );
    EXPECT_EQ( floating_point_number, 66.6600031f );
}

TEST( text_string_input, parse_multiple_values_error )
{
    String_view str( "  text  66.66     12  " );
    Text_string_input text_string_input( str );

    String_view substring;
    uint32_t integer_number = 0u;
    float floating_point_number = 0.0f;

    EXPECT_FALSE( text_string_input >> substring >> integer_number >> floating_point_number );
    EXPECT_EQ( substring, "text" );
    EXPECT_EQ( integer_number, 0u );
    EXPECT_EQ( floating_point_number, 0.0f );

    EXPECT_TRUE( text_string_input >> floating_point_number >> integer_number );
    EXPECT_EQ( integer_number, 12 );
    EXPECT_EQ( floating_point_number, 66.6600031f );
}

TEST( text_string_input, get_line )
{
    String_view str( "  text 12    66.66   " );
    Text_string_input text_string_input( str );

    String_view substring;

    EXPECT_TRUE( text_string_input >> substring );

    String_view reminder = text_string_input.get_line();

    EXPECT_EQ( reminder, " 12    66.66   " );

    EXPECT_FALSE( text_string_input >> substring );

    reminder = text_string_input.get_line();
    EXPECT_EQ( reminder, "" );
}

TEST( text_string_input, delimiter )
{
    String_view str( "/this/ is a /path  " );
    Text_string_input text_string_input( str );

    String_view substring;

    substring = text_string_input.get_line( '/' );
    EXPECT_EQ( substring, "" );

    substring = text_string_input.get_line( '/' );
    EXPECT_EQ( substring, "this" );

    substring = text_string_input.get_line( '/' );
    EXPECT_EQ( substring, " is a " );

    substring = text_string_input.get_line( '/' );
    EXPECT_EQ( substring, "path  " );

    EXPECT_FALSE( text_string_input >> substring );
}

TEST( text_string_input, single_character )
{
    String_view str( "?" );
    Text_string_input text_string_input( str );
    String_view substring;

    EXPECT_TRUE( text_string_input >> substring );
    EXPECT_EQ( substring, "?" );
}

TEST( text_string_input, single_character_number )
{
    String_view str( "9" );
    Text_string_input text_string_input( str );
    uint32_t integer_number;

    EXPECT_TRUE( text_string_input >> integer_number );
    EXPECT_EQ( integer_number, 9 );
}
