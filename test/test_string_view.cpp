// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cinttypes>
#include <limits>

#include "test_common.h"

#include "soma/text/string_view.h"

using namespace SOMA;

TEST( string_view, comparison )
{
    String_view empty_str( "" );

    EXPECT_TRUE( empty_str == "" );

    String_view str( "tested string" );

    EXPECT_TRUE( str == "tested string" );
    EXPECT_FALSE( str != "tested string" );

    EXPECT_FALSE( str == "different string" );
    EXPECT_TRUE( str != "different string" );

    EXPECT_FALSE( str == "tested" );
    EXPECT_TRUE( str != "tested" );
}

TEST( string_view, comparison_partial )
{
    String_view str { "tested string" };

    EXPECT_FALSE( str == "tested" );
    EXPECT_TRUE( str != "tested" );

    EXPECT_FALSE( str == "test" );
    EXPECT_TRUE( str != "test" );
}

TEST( string_view, buffer_property )
{
    {
        String_view str { "" };

        EXPECT_TRUE( str.is_empty() );
        EXPECT_EQ( str.length(), 0 );
    }

    {
        String_view str { "tested" };

        EXPECT_FALSE( str.is_empty() );
        EXPECT_EQ( str.length(), 6 );
    }
}

TEST( string_view, from_string_unsigned )
{
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint64_t value64;

    {
        String_view str( "-13" );

        EXPECT_FALSE( str.to( value8 ) );
    }

    {
        // checks underflow happening in strtoul/strtoull that can result in false positive conversion
        String_view str { "-4294967169" };

        EXPECT_FALSE( str.to( value8 ) );
    }

    {
        String_view str { "13" };

        EXPECT_TRUE( str.to( value8 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        String_view str { "400" };

        EXPECT_FALSE( str.to( value8 ) );

        EXPECT_TRUE( str.to( value16 ) );
        EXPECT_EQ( value16, 400 );

        EXPECT_TRUE( str.to( value32 ) );
        EXPECT_EQ( value32, 400 );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, 400 );
    }

    {
        String_view str { "70000" };

        EXPECT_FALSE( str.to( value16 ) );

        EXPECT_TRUE( str.to( value32 ) );
        EXPECT_EQ( value32, 70000 );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, 70000 );
    }

    {
        String_view str { "5000000000" };

        EXPECT_FALSE( str.to( value32 ) );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, 5000000000 );
    }
}

TEST( string_view, from_string_unsigned_hex )
{
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint64_t value64;

    {
        // Test with "0x" prefix
        String_view str { "0x0D" };

        EXPECT_TRUE( str.to( value8, 16 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        // Test with "0X" prefix (uppercase)
        String_view str { "0X1A" };

        EXPECT_TRUE( str.to( value8, 16 ) );
        EXPECT_EQ( value8, 26 );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 26 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 26 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 26 );
    }

    {
        // Test without prefix (lowercase)
        String_view str { "ff" };

        EXPECT_TRUE( str.to( value8, 16 ) );
        EXPECT_EQ( value8, 255 );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 255 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 255 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 255 );
    }

    {
        // Test without prefix (uppercase)
        String_view str { "FF" };

        EXPECT_TRUE( str.to( value8, 16 ) );
        EXPECT_EQ( value8, 255 );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 255 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 255 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 255 );
    }

    {
        // Test overflow for uint8_t
        String_view str { "0x100" };

        EXPECT_FALSE( str.to( value8, 16 ) );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 256 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 256 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 256 );
    }

    {
        // Test overflow for uint16_t
        String_view str { "0x10000" };

        EXPECT_FALSE( str.to( value16, 16 ) );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 65536 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 65536 );
    }

    {
        // Test overflow for uint32_t
        String_view str { "0x100000000" };

        EXPECT_FALSE( str.to( value32, 16 ) );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 0x100000000ULL );
    }

    {
        // Test invalid characters
        String_view str { "0xGG" };

        EXPECT_FALSE( str.to( value8, 16 ) );
        EXPECT_FALSE( str.to( value16, 16 ) );
        EXPECT_FALSE( str.to( value32, 16 ) );
        EXPECT_FALSE( str.to( value64, 16 ) );
    }

    {
        // Test negative sign (should fail for unsigned)
        String_view str { "-0xA" };

        EXPECT_FALSE( str.to( value8, 16 ) );
        EXPECT_FALSE( str.to( value16, 16 ) );
        EXPECT_FALSE( str.to( value32, 16 ) );
        EXPECT_FALSE( str.to( value64, 16 ) );
    }

    {
        // Test mixed case
        String_view str { "0xAbCd" };

        EXPECT_FALSE( str.to( value8, 16 ) );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 0xABCD );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 0xABCD );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 0xABCD );
    }
}

TEST( string_view, from_string_unsigned_octal )
{
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint64_t value64;

    {
        // Test with "0" prefix (octal)
        String_view str { "015" };

        EXPECT_TRUE( str.to( value8, 8 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        // Test without prefix
        String_view str { "77" };

        EXPECT_TRUE( str.to( value8, 8 ) );
        EXPECT_EQ( value8, 63 );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 63 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 63 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 63 );
    }

    {
        // Test overflow for uint8_t (377 octal = 255 decimal, 400 octal = 256 decimal)
        String_view str { "0400" };

        EXPECT_FALSE( str.to( value8, 8 ) );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 256 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 256 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 256 );
    }

    {
        // Test invalid characters for octal (8 and 9 are invalid)
        String_view str { "089" };

        EXPECT_FALSE( str.to( value8, 8 ) );
        EXPECT_FALSE( str.to( value16, 8 ) );
        EXPECT_FALSE( str.to( value32, 8 ) );
        EXPECT_FALSE( str.to( value64, 8 ) );
    }

    {
        // Test negative sign (should fail for unsigned)
        String_view str { "-077" };

        EXPECT_FALSE( str.to( value8, 8 ) );
        EXPECT_FALSE( str.to( value16, 8 ) );
        EXPECT_FALSE( str.to( value32, 8 ) );
        EXPECT_FALSE( str.to( value64, 8 ) );
    }

    {
        // Test maximum octal value for uint8_t (377 octal = 255 decimal)
        String_view str { "0377" };

        EXPECT_TRUE( str.to( value8, 8 ) );
        EXPECT_EQ( value8, 255 );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 255 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 255 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 255 );
    }
}

TEST( string_view, from_string_unsigned_binary )
{
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint64_t value64;

    {
        // Test with "0b" prefix (binary)
        String_view str { "0b1101" };

        EXPECT_TRUE( str.to( value8, 2 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        // Test with "0B" prefix (uppercase)
        String_view str { "0B1010" };

        EXPECT_TRUE( str.to( value8, 2 ) );
        EXPECT_EQ( value8, 10 );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 10 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 10 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 10 );
    }

    {
        // Test without prefix
        String_view str { "11111111" };

        EXPECT_TRUE( str.to( value8, 2 ) );
        EXPECT_EQ( value8, 255 );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 255 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 255 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 255 );
    }

    {
        // Test overflow for uint8_t (9 bits)
        String_view str { "0b100000000" };

        EXPECT_FALSE( str.to( value8, 2 ) );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 256 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 256 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 256 );
    }

    {
        // Test invalid characters for binary (2-9 are invalid)
        String_view str { "0b102" };

        EXPECT_FALSE( str.to( value8, 2 ) );
        EXPECT_FALSE( str.to( value16, 2 ) );
        EXPECT_FALSE( str.to( value32, 2 ) );
        EXPECT_FALSE( str.to( value64, 2 ) );
    }

    {
        // Test negative sign (should fail for unsigned)
        String_view str { "-0b1010" };

        EXPECT_FALSE( str.to( value8, 2 ) );
        EXPECT_FALSE( str.to( value16, 2 ) );
        EXPECT_FALSE( str.to( value32, 2 ) );
        EXPECT_FALSE( str.to( value64, 2 ) );
    }

    {
        // Test edge case: just "0b" without digits
        String_view str { "0b" };

        EXPECT_FALSE( str.to( value8, 2 ) );
        EXPECT_FALSE( str.to( value16, 2 ) );
        EXPECT_FALSE( str.to( value32, 2 ) );
        EXPECT_FALSE( str.to( value64, 2 ) );
    }

    {
        // Test single bit values
        String_view str_zero { "0b0" };
        String_view str_one { "0b1" };

        EXPECT_TRUE( str_zero.to( value8, 2 ) );
        EXPECT_EQ( value8, 0 );

        EXPECT_TRUE( str_one.to( value8, 2 ) );
        EXPECT_EQ( value8, 1 );
    }
}

TEST( string_view, from_string_signed )
{
    int8_t value8;
    int16_t value16;
    int32_t value32;
    int64_t value64;

    {
        String_view str { "-13" };


        EXPECT_TRUE( str.to( value8 ) );
        EXPECT_EQ( value8, -13 );

        EXPECT_TRUE( str.to( value16 ) );
        EXPECT_EQ( value16, -13 );

        EXPECT_TRUE( str.to( value32 ) );
        EXPECT_EQ( value32, -13 );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, -13 );
    }

    {
        String_view str { "-200" };

        EXPECT_FALSE( str.to( value8 ) );

        EXPECT_TRUE( str.to( value16 ) );
        EXPECT_EQ( value16, -200 );

        EXPECT_TRUE( str.to( value32 ) );
        EXPECT_EQ( value32, -200 );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, -200 );
    }

    {
        String_view str { "-40000" };

        EXPECT_FALSE( str.to( value16 ) );

        EXPECT_TRUE( str.to( value32 ) );
        EXPECT_EQ( value32, -40000 );

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, -40000 );
    }

    {
        String_view str { "-3000000000" };

        EXPECT_FALSE( str.to( value32 ) ); // out of range

        EXPECT_TRUE( str.to( value64 ) );
        EXPECT_EQ( value64, -3000000000 );
    }
}

TEST( string_view, from_string_signed_hex )
{
    int8_t value8;
    int16_t value16;
    int32_t value32;
    int64_t value64;

    {
        // Test with "0x" prefix (positive)
        String_view str { "0x0D" };

        EXPECT_TRUE( str.to( value8, 16 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        // Test with negative hex
        String_view str { "-0x7F" };

        EXPECT_TRUE( str.to( value8, 16 ) );
        EXPECT_EQ( value8, -127 );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, -127 );

        EXPECT_TRUE( str.to( value32, 16 ) );
        EXPECT_EQ( value32, -127 );

        EXPECT_TRUE( str.to( value64, 16 ) );
        EXPECT_EQ( value64, -127 );
    }

    {
        // Test overflow for int8_t
        String_view str { "0x80" };

        EXPECT_FALSE( str.to( value8, 16 ) );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, 128 );
    }

    {
        // Test negative overflow for int8_t
        String_view str { "-0x81" };

        EXPECT_FALSE( str.to( value8, 16 ) );

        EXPECT_TRUE( str.to( value16, 16 ) );
        EXPECT_EQ( value16, -129 );
    }
}

TEST( string_view, from_string_signed_octal )
{
    int8_t value8;
    int16_t value16;
    int32_t value32;
    int64_t value64;

    {
        // Test with "0" prefix (positive octal)
        String_view str { "015" };

        EXPECT_TRUE( str.to( value8, 8 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        // Test with negative octal
        String_view str { "-077" };

        EXPECT_TRUE( str.to( value8, 8 ) );
        EXPECT_EQ( value8, -63 );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, -63 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, -63 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, -63 );
    }

    {
        // Test overflow for int8_t (200 octal = 128 decimal)
        String_view str { "0200" };

        EXPECT_FALSE( str.to( value8, 8 ) );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 128 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 128 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 128 );
    }

    {
        // Test negative overflow for int8_t
        String_view str { "-0201" };

        EXPECT_FALSE( str.to( value8, 8 ) );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, -129 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, -129 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, -129 );
    }

    {
        // Test maximum positive value for int8_t (177 octal = 127 decimal)
        String_view str { "0177" };

        EXPECT_TRUE( str.to( value8, 8 ) );
        EXPECT_EQ( value8, 127 );

        EXPECT_TRUE( str.to( value16, 8 ) );
        EXPECT_EQ( value16, 127 );

        EXPECT_TRUE( str.to( value32, 8 ) );
        EXPECT_EQ( value32, 127 );

        EXPECT_TRUE( str.to( value64, 8 ) );
        EXPECT_EQ( value64, 127 );
    }

    {
        // Test invalid characters for octal (8 and 9 are invalid)
        String_view str { "089" };

        EXPECT_FALSE( str.to( value8, 8 ) );
        EXPECT_FALSE( str.to( value16, 8 ) );
        EXPECT_FALSE( str.to( value32, 8 ) );
        EXPECT_FALSE( str.to( value64, 8 ) );
    }
}

TEST( string_view, from_string_signed_binary )
{
    int8_t value8;
    int16_t value16;
    int32_t value32;
    int64_t value64;

    {
        // Test with "0b" prefix (positive binary)
        String_view str { "0b1101" };

        EXPECT_TRUE( str.to( value8, 2 ) );
        EXPECT_EQ( value8, 13 );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 13 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 13 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 13 );
    }

    {
        // Test with "0B" prefix (uppercase, negative)
        String_view str { "-0B1010" };

        EXPECT_TRUE( str.to( value8, 2 ) );
        EXPECT_EQ( value8, -10 );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, -10 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, -10 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, -10 );
    }

    {
        // Test maximum positive value for int8_t (1111111 binary = 127 decimal)
        String_view str { "0b1111111" };

        EXPECT_TRUE( str.to( value8, 2 ) );
        EXPECT_EQ( value8, 127 );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 127 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 127 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 127 );
    }

    {
        // Test overflow for int8_t (10000000 binary = 128 decimal)
        String_view str { "0b10000000" };

        EXPECT_FALSE( str.to( value8, 2 ) );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, 128 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, 128 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, 128 );
    }

    {
        // Test negative overflow for int8_t
        String_view str { "-0b10000001" };

        EXPECT_FALSE( str.to( value8, 2 ) );

        EXPECT_TRUE( str.to( value16, 2 ) );
        EXPECT_EQ( value16, -129 );

        EXPECT_TRUE( str.to( value32, 2 ) );
        EXPECT_EQ( value32, -129 );

        EXPECT_TRUE( str.to( value64, 2 ) );
        EXPECT_EQ( value64, -129 );
    }

    {
        // Test invalid characters for binary (2-9 are invalid)
        String_view str { "0b102" };

        EXPECT_FALSE( str.to( value8, 2 ) );
        EXPECT_FALSE( str.to( value16, 2 ) );
        EXPECT_FALSE( str.to( value32, 2 ) );
        EXPECT_FALSE( str.to( value64, 2 ) );
    }

    {
        // Test edge case: just "0b" without digits
        String_view str { "0b" };

        EXPECT_FALSE( str.to( value8, 2 ) );
        EXPECT_FALSE( str.to( value16, 2 ) );
        EXPECT_FALSE( str.to( value32, 2 ) );
        EXPECT_FALSE( str.to( value64, 2 ) );
    }

    {
        // Test single bit values
        String_view str_zero { "0b0" };
        String_view str_one { "0b1" };
        String_view str_neg_one { "-0b1" };

        EXPECT_TRUE( str_zero.to( value8, 2 ) );
        EXPECT_EQ( value8, 0 );

        EXPECT_TRUE( str_one.to( value8, 2 ) );
        EXPECT_EQ( value8, 1 );

        EXPECT_TRUE( str_neg_one.to( value8, 2 ) );
        EXPECT_EQ( value8, -1 );
    }
}

TEST( string_view, from_string_floating )
{
    String_view str { "-13.0345" };
    float value_f;
    double value_d;

    EXPECT_TRUE( str.to( value_f ) );
    EXPECT_EQ( value_f, -13.0345001f );

    EXPECT_TRUE( str.to( value_d ) );
    EXPECT_EQ( value_d, -13.0345 );
}

TEST( string_view, numeric_conversion_rejects_overflow_and_partial_input )
{
    uint32_t unsigned_value = 7;
    EXPECT_FALSE( String_view( "4294967296" ).to( unsigned_value ) );
    EXPECT_EQ( unsigned_value, 7u );

    int32_t signed_value = 7;
    EXPECT_TRUE( String_view( "-2147483648" ).to( signed_value ) );
    EXPECT_EQ( signed_value, std::numeric_limits<int32_t>::min() );
    EXPECT_FALSE( String_view( "-2147483649" ).to( signed_value ) );
    EXPECT_EQ( signed_value, std::numeric_limits<int32_t>::min() );

    float float_value = 7.0f;
    EXPECT_FALSE( String_view( "2.5junk" ).to( float_value ) );
    EXPECT_FALSE( String_view( "nan" ).to( float_value ) );
    EXPECT_EQ( float_value, 7.0f );

    const char characters[] = { '2', '.', '5', '9' };
    EXPECT_TRUE( String_view( characters, 3 ).to( float_value ) );
    EXPECT_EQ( float_value, 2.5f );
}

TEST( string_view, iteration )
{
    String_view str { "test" };

    size_t i = 0;

    for( auto ch: str )
    {
        switch( i )
        {
            case 0:
                ASSERT_EQ( ch, 't' );
                break;

            case 1:
                ASSERT_EQ( ch, 'e' );
                break;

            case 2:
                ASSERT_EQ( ch, 's' );
                break;

            case 3:
                ASSERT_EQ( ch, 't' );
                break;

            default:
                ASSERT_TRUE( false );
                break;
        }

        ++i;
    }
}

TEST( string_view, reversed_iteration )
{
    String_view str { "junk" };

    size_t i = 0;

    for( auto ch: Reverse_iterator( str ) )
    {
        switch( i )
        {
            case 0:
                ASSERT_EQ( ch, 'k' );
                break;

            case 1:
                ASSERT_EQ( ch, 'n' );
                break;

            case 2:
                ASSERT_EQ( ch, 'u' );
                break;

            case 3:
                ASSERT_EQ( ch, 'j' );
                break;

            default:
                ASSERT_TRUE( false );
                break;
        }

        ++i;
    }
}

TEST( string_view, starts_with )
{
    String_view str { "teststring" };

    // Test starts_with(const char*)
    ASSERT_TRUE( str.starts_with( "tes" ) );
    ASSERT_FALSE( str.starts_with( "str" ) );
    ASSERT_TRUE( str.starts_with( "test" ) );
    ASSERT_FALSE( str.starts_with( "string" ) );
    ASSERT_TRUE( str.starts_with( "teststring" ) );  // entire string
    ASSERT_FALSE( str.starts_with( "verylongteststring" ) );  // longer than string

    // Test starts_with(char)
    ASSERT_TRUE( str.starts_with( 't' ) );
    ASSERT_FALSE( str.starts_with( 'e' ) );
    ASSERT_FALSE( str.starts_with( 'g' ) );

    // Test starts_with(String_view)
    String_view tes { "tes" };
    ASSERT_TRUE( str.starts_with( tes ) );

    String_view ing { "ing" };
    ASSERT_FALSE( str.starts_with( ing ) );

    String_view test_prefix { "test" };
    ASSERT_TRUE( str.starts_with( test_prefix ) );

    String_view entire_string { "teststring" };
    ASSERT_TRUE( str.starts_with( entire_string ) );

    String_view too_long { "verylongteststring" };
    ASSERT_FALSE( str.starts_with( too_long ) );

    // Test with empty string
    String_view empty_str { "" };
    String_view empty_prefix { "" };
    ASSERT_TRUE( empty_str.starts_with( "" ) );
    ASSERT_TRUE( empty_str.starts_with( empty_prefix ) );
    ASSERT_FALSE( empty_str.starts_with( 'a' ) );
    ASSERT_FALSE( empty_str.starts_with( "non-empty" ) );

    // Test non-empty string with empty prefix
    ASSERT_TRUE( str.starts_with( "" ) );
    ASSERT_TRUE( str.starts_with( empty_prefix ) );
}

TEST( string_view, ends_with )
{
    String_view str { "teststring" };

    // Test ends_with(const char*)
    ASSERT_TRUE( str.ends_with( "ing" ) );
    ASSERT_FALSE( str.ends_with( "tes" ) );
    ASSERT_TRUE( str.ends_with( "string" ) );
    ASSERT_FALSE( str.ends_with( "test" ) );
    ASSERT_TRUE( str.ends_with( "teststring" ) );  // entire string
    ASSERT_FALSE( str.ends_with( "toolongstring" ) );  // longer than string

    // Test ends_with(char)
    ASSERT_TRUE( str.ends_with( 'g' ) );
    ASSERT_FALSE( str.ends_with( 't' ) );
    ASSERT_FALSE( str.ends_with( 'e' ) );

    // Test ends_with(String_view)
    String_view ing { "ing" };
    ASSERT_TRUE( str.ends_with( ing ) );

    String_view tes { "tes" };
    ASSERT_FALSE( str.ends_with( tes ) );

    String_view string_suffix { "string" };
    ASSERT_TRUE( str.ends_with( string_suffix ) );

    String_view entire_string { "teststring" };
    ASSERT_TRUE( str.ends_with( entire_string ) );

    String_view too_long { "verylongteststring" };
    ASSERT_FALSE( str.ends_with( too_long ) );

    // Test with empty string
    String_view empty_str { "" };
    String_view empty_suffix { "" };
    ASSERT_TRUE( empty_str.ends_with( "" ) );
    ASSERT_TRUE( empty_str.ends_with( empty_suffix ) );
    ASSERT_FALSE( empty_str.ends_with( 'a' ) );
    ASSERT_FALSE( empty_str.ends_with( "non-empty" ) );

    // Test non-empty string with empty suffix
    ASSERT_TRUE( str.ends_with( "" ) );
    ASSERT_TRUE( str.ends_with( empty_suffix ) );
}

TEST( string_view, trimming )
{
    String_view str { "   teststring   " };
    String_view trimmed_left = str.trim_left();
    String_view trimmed_right = str.trim_right();
    String_view trimmed = str.trim();

    ASSERT_EQ( trimmed_left, "teststring   " );
    ASSERT_EQ( trimmed_right, "   teststring" );
    ASSERT_EQ( trimmed, "teststring" );
}

TEST( string_view, trimming_no_effect )
{
    String_view str { "teststring" };
    String_view trimmed_left = str.trim_left();
    String_view trimmed_right = str.trim_right();
    String_view trimmed = str.trim();

    ASSERT_EQ( trimmed_left, "teststring" );
    ASSERT_EQ( trimmed_right, "teststring" );
    ASSERT_EQ( trimmed, "teststring" );
}

TEST( string_view, find_substring )
{
    String_view str { "hello world" };
    size_t pos = 0;

    // substring at beginning
    ASSERT_TRUE( str.find( String_view( "hello" ), pos ) );
    ASSERT_EQ( pos, 0u );

    // substring in the middle
    ASSERT_TRUE( str.find( String_view( "lo wo" ), pos ) );
    ASSERT_EQ( pos, 3u );

    // substring at the end
    ASSERT_TRUE( str.find( String_view( "world" ), pos ) );
    ASSERT_EQ( pos, 6u );

    // substring not found
    ASSERT_FALSE( str.find( String_view( "notfound" ), pos ) );

    // substring is the whole string
    ASSERT_TRUE( str.find( String_view( "hello world" ), pos ) );
    ASSERT_EQ( pos, 0u );

    // substring is empty
    ASSERT_FALSE( str.find( String_view( "" ), pos ) );

    // substring longer than string
    ASSERT_FALSE( str.find( String_view( "hello world!" ), pos ) );
}

TEST( string_view, find_character )
{
    String_view str { "abcdefgabc" };
    size_t pos = 0;

    // character at beginning
    ASSERT_TRUE( str.find( 'a', pos ) );
    ASSERT_EQ( pos, 0u );

    // character in the middle
    ASSERT_TRUE( str.find( 'd', pos ) );
    ASSERT_EQ( pos, 3u );

    // character at the end
    ASSERT_TRUE( str.find( 'c', pos ) );
    ASSERT_EQ( pos, 2u ); // first occurrence

    // character not found
    ASSERT_FALSE( str.find( 'z', pos ) );

    // character appears multiple times, should return first
    ASSERT_TRUE( str.find( 'b', pos ) );
    ASSERT_EQ( pos, 1u );

    // empty string
    String_view empty_str { "" };

    ASSERT_FALSE( empty_str.find( 'a', pos ) );
}

TEST( string_view, is_single_word )
{
    String_view empty_str( "" );
    EXPECT_TRUE( empty_str.is_single_word() );

    String_view single_word( "Hello" );

    EXPECT_TRUE( single_word.is_single_word() );

    String_view with_space( "Hello World" );

    EXPECT_FALSE( with_space.is_single_word() );

    String_view with_tab( "Hello\tWorld" );

    EXPECT_FALSE( with_tab.is_single_word() );

    String_view with_newline( "Hello\nWorld" );

    EXPECT_FALSE( with_newline.is_single_word() );

    String_view only_space( "   " );

    EXPECT_FALSE( only_space.is_single_word() );

    String_view leading_space( " Hello" );

    EXPECT_FALSE( leading_space.is_single_word() );

    String_view trailing_space( "Hello " );

    EXPECT_FALSE( trailing_space.is_single_word() );
}

TEST( string_view, to_integer_base_auto_detection )
{
    // Test decimal (default)
    {
        String_view str( "123" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value ) );  // base 0 (auto-detect) should default to 10
        EXPECT_EQ( value, 123u );
    }

    // Test hexadecimal detection
    {
        String_view str( "0xFF" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value ) );  // base 0 (auto-detect) should detect hex
        EXPECT_EQ( value, 255u );
    }

    {
        String_view str( "0x1A" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value ) );
        EXPECT_EQ( value, 26u );
    }

    // Test binary detection
    {
        String_view str( "0b1010" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value ) );  // base 0 (auto-detect) should detect binary
        EXPECT_EQ( value, 10u );
    }

    {
        String_view str( "0B1111" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value ) );
        EXPECT_EQ( value, 15u );
    }

    // Test octal detection
    {
        String_view str( "0755" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value ) );  // base 0 (auto-detect) should detect octal
        EXPECT_EQ( value, 493u );  // 7*64 + 5*8 + 5 = 448 + 40 + 5 = 493
    }

    // Test with signed integers
    {
        String_view str( "-0x10" );
        int32_t value = 0;
        EXPECT_TRUE( str.to( value ) );
        EXPECT_EQ( value, -16 );
    }

    {
        String_view str( "+0b1000" );
        int32_t value = 0;
        EXPECT_TRUE( str.to( value ) );
        EXPECT_EQ( value, 8 );
    }

    // Test that explicit base still works
    {
        String_view str( "FF" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value, 16 ) );  // explicit base 16
        EXPECT_EQ( value, 255u );
    }

    {
        String_view str( "1010" );
        uint32_t value = 0;
        EXPECT_TRUE( str.to( value, 2 ) );  // explicit base 2
        EXPECT_EQ( value, 10u );
    }

    // Test edge cases
    {
        String_view str( "0" );
        uint32_t value = 42; // Initialize with non-zero
        EXPECT_TRUE( str.to( value ) );  // Should work with base auto-detection
        EXPECT_EQ( value, 0u );
    }

    {
        String_view str( "0x" );
        uint32_t value = 0;
        EXPECT_FALSE( str.to( value ) );  // Invalid hex - missing digits
    }

    {
        String_view str( "0b" );
        uint32_t value = 0;
        EXPECT_FALSE( str.to( value ) );  // Invalid binary - missing digits
    }
}

TEST( string_view, to_hex_bytes )
{
    // Test basic conversion
    {
        String_view str( "0xAABBCC" );
        uint8_t storage[3];
        Array<uint8_t> output( storage, 3 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );
        EXPECT_EQ( output[0], 0xAA );
        EXPECT_EQ( output[1], 0xBB );
        EXPECT_EQ( output[2], 0xCC );
    }

    // Test lowercase hex
    {
        String_view str( "0xaabbcc" );
        uint8_t storage[3];
        Array<uint8_t> output( storage, 3 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );
        EXPECT_EQ( output[0], 0xAA );
        EXPECT_EQ( output[1], 0xBB );
        EXPECT_EQ( output[2], 0xCC );
    }

    // Test mixed case hex
    {
        String_view str( "0xAbCd" );
        uint8_t storage[2];
        Array<uint8_t> output( storage, 2 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );
        EXPECT_EQ( output[0], 0xAB );
        EXPECT_EQ( output[1], 0xCD );
    }

    // Test single byte
    {
        String_view str( "0xFF" );
        uint8_t storage[1];
        Array<uint8_t> output( storage, 1 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );
        EXPECT_EQ( output[0], 0xFF );
    }

    // Test zeros
    {
        String_view str( "0x000000" );
        uint8_t storage[3];
        Array<uint8_t> output( storage, 3 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );
        EXPECT_EQ( output[0], 0x00 );
        EXPECT_EQ( output[1], 0x00 );
        EXPECT_EQ( output[2], 0x00 );
    }

    // Test missing 0x prefix
    {
        String_view str( "AABBCC" );
        uint8_t storage[3];
        Array<uint8_t> output( storage, 3 );

        EXPECT_FALSE( str.to_hex_bytes( output ) );
    }

    // Test odd number of hex digits
    {
        String_view str( "0xAAB" );
        uint8_t storage[2];
        Array<uint8_t> output( storage, 2 );

        EXPECT_FALSE( str.to_hex_bytes( output ) );
    }

    // Test insufficient array capacity
    {
        String_view str( "0xAABBCC" );
        uint8_t storage[2];  // Only 2 bytes, but need 3
        Array<uint8_t> output( storage, 2 );

        EXPECT_FALSE( str.to_hex_bytes( output ) );
    }

    // Test invalid hex characters
    {
        String_view str( "0xAAGGCC" );
        uint8_t storage[3];
        Array<uint8_t> output( storage, 3 );

        EXPECT_FALSE( str.to_hex_bytes( output ) );
    }

    // Test empty hex string
    {
        String_view str( "0x" );
        uint8_t storage[1];
        Array<uint8_t> output( storage, 1 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );  // Valid - zero bytes
    }

    // Test larger array than needed
    {
        String_view str( "0x1122" );
        uint8_t storage[10];  // Much larger than needed
        Array<uint8_t> output( storage, 10 );

        EXPECT_TRUE( str.to_hex_bytes( output ) );
        EXPECT_EQ( output[0], 0x11 );
        EXPECT_EQ( output[1], 0x22 );
    }
}

