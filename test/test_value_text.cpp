// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"
#include "soma/text/text_string_output.h"
#include "soma/variables/overridable_variable.h"

using namespace SOMA;

namespace Value_text_test
{
    enum class Mode { idle, active };

    Text_output& operator<<( Text_output& output, Mode value )
    {
        return output << ( value == Mode::idle ? "idle" : "active" );
    }

    struct Parsed_value
    {
        int number;
        bool operator==( const Parsed_value& rhs ) const { return number == rhs.number; }
    };

    Text_output& operator<<( Text_output& output, const Parsed_value& value )
    {
        return output << value.number;
    }

    bool from_text( String_view text, Parsed_value& value )
    {
        value.number = 999; // A failing parser may modify its output argument.
        if( text == "valid" )
        {
            value.number = 42;
            return true;
        }
        return false;
    }
}

TEST( value_text, plain_variable_and_base_share_formatter )
{
    using namespace Value_text_test;
    Variable<Mode> mode { Mode::active };
    const B_variable& base = mode;
    String::Length<100> text;
    Text_string_output output { text };

    output << Mode::active << "/" << mode << "/" << base;
    EXPECT_TRUE( text.view() == "active/active/active" );
}

TEST( value_text, arithmetic_and_custom_values_preserve_formatting_and_chaining )
{
    using namespace Value_text_test;
    Variable<int> number { 42 };
    Variable<Parsed_value> custom { Parsed_value { 42 } };
    const B_variable& base = custom;
    String::Length<100> text;
    Text_string_output output { text };

    output << hex << number << "/" << min_width( 4 ) << fill_with( '0' ) << custom
           << "/" << base << "/" << Parsed_value { 42 };
    EXPECT_TRUE( text.view() == "2A/0042/42/42" );
}

TEST( value_text, failed_custom_parse_preserves_existing_override )
{
    using namespace Value_text_test;
    Overridable_variable<Parsed_value> variable { Parsed_value { 1 } };

    EXPECT_FALSE( variable.set_override( String_view { "invalid" } ) );
    EXPECT_FALSE( variable.is_overridden() );
    EXPECT_EQ( variable.get().number, 1 );
    ASSERT_TRUE( variable.set_override( String_view { "valid" } ) );
    EXPECT_EQ( variable.get().number, 42 );
    EXPECT_FALSE( variable.set_override( String_view { "invalid" } ) );
    EXPECT_TRUE( variable.is_overridden() );
    EXPECT_EQ( variable.get().number, 42 );
    variable.clear_override();
    EXPECT_EQ( variable.get().number, 1 );
}

TEST( value_text, built_in_parsers_are_available )
{
    int number = 0;
    bool flag = false;
    EXPECT_TRUE( from_text( String_view { "42" }, number ) );
    EXPECT_EQ( number, 42 );
    EXPECT_TRUE( from_text( String_view { "true" }, flag ) );
    EXPECT_TRUE( flag );
    EXPECT_FALSE( from_text( String_view { "invalid" }, flag ) );
}
