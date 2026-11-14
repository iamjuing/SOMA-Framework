// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cinttypes>
#include <limits>

#include "test_common.h"

#include "soma/text/string.h"

using namespace SOMA;

TEST( string, external_and_owned_storage )
{
    char external[6];
    String view { Buffer<char>{ external } };
    String::Length<5> first { "abc" };
    String::Length<5> second;
    String::Length<0> empty;

    EXPECT_EQ( view.capacity(), 5u );
    EXPECT_EQ( first.capacity(), 5u );
    EXPECT_EQ( empty.capacity(), 0u );
    EXPECT_TRUE( empty.is_empty() );
    EXPECT_TRUE( empty.is_full() );
    EXPECT_STREQ( empty.C_string(), "" );

    String& common = first;
    common += 'd';
    view = common.view();
    second = "xy";

    EXPECT_STREQ( first.C_string(), "abcd" );
    EXPECT_STREQ( external, "abcd" );
    EXPECT_STREQ( second.C_string(), "xy" );

    first.clear();
    EXPECT_STREQ( external, "abcd" );
    EXPECT_STREQ( second.C_string(), "xy" );
}

TEST( string, buffer_property )
{
    String::Length<20> str;

    EXPECT_TRUE( str.is_empty() );
    EXPECT_FALSE( str.is_full() );
    EXPECT_EQ( str.length(), 0 );
    EXPECT_EQ( str.space(), 20 );
    EXPECT_EQ( str.capacity(), 20 );

    str = "tested";

    EXPECT_FALSE( str.is_empty() );
    EXPECT_FALSE( str.is_full() );
    EXPECT_EQ( str.length(), 6 );
    EXPECT_EQ( str.space(), 14 );

    EXPECT_EQ( str.capacity(), 20 );

    str += " string";

    EXPECT_FALSE( str.is_empty() );
    EXPECT_FALSE( str.is_full() );
    EXPECT_EQ( str.length(), 13 );
    EXPECT_EQ( str.space(), 7 );

    str += " buffer certainly full";

    EXPECT_FALSE( str.is_empty() );
    EXPECT_TRUE( str.is_full() );
    EXPECT_EQ( str.length(), 20 );
    EXPECT_EQ( str.space(), 0 );

    EXPECT_TRUE( str.view() == "tested string buffer" );
}

TEST( string, fraction )
{
    String::Length<20> str;

    str.fraction( 23u, 4188u, "%3" PRIu32, 2 );
    EXPECT_EQ( str.view(), "  0.01" );

    str.clear();

    str.fraction( 2043u, 503902u, "%3" PRIu32, 2 );
    EXPECT_EQ( str.view(), "  0.00" );

    str.clear();

    str.fraction( 1556u, 23u, "%3" PRIu32, 2 );
    EXPECT_EQ( str.view(), " 67.65" );

    str.clear();

    str.fraction( 1556u, 23u, "%" PRIu32, 2 );
    EXPECT_EQ( str.view(), "67.65" );

    str.clear();

    str.fraction( 1556u, 23u, "%4" PRIu32, 1 );
    EXPECT_EQ( str.view(), "  67.7" );

    str.clear();

    str.fraction( 1556, 23, "%1" PRIu32, 5 );
    EXPECT_EQ( str.view(), "67.65217" );

    str.clear();

    str.fraction( 1556, 23, "%1" PRIu32, 0 );
    EXPECT_EQ( str.view(), "68" );

    str.clear();

    str.fraction( -1556, 23, "%" PRId32, 2 );
    EXPECT_EQ( str.view(), "-67.65" );

    str.clear();

    str.fraction( -1556, 23, "%4" PRId32, 2 );
    EXPECT_EQ( str.view(), " -67.65" );

    str.clear();

    str.fraction( -1556, 23, "%5" PRId32, 2 );
    EXPECT_EQ( str.view(), "  -67.65" );

    str.clear();

    str.fraction( -1556, 23, "%" PRId32, 1 );
    EXPECT_EQ( str.view(), "-67.7" );

    str.clear();

    str.fraction( -1556, 23, "%" PRId32, 0 );
    EXPECT_EQ( str.view(), "-68" );

    str.clear();

    str.fraction( -1556, -23, "%" PRId32, 1 );
    EXPECT_EQ( str.view(), "67.7" );

    str.clear();

    str.fraction( 1556, -23, "%" PRId32, 1 );
    EXPECT_EQ( str.view(), "-67.7" );

    str.clear();

    str.fraction( 5, 2, "%3" PRId32, 3 );
    EXPECT_EQ( str.view(), "  2.500" );

    str.clear();

    str.fraction( 5, 2, "%" PRId32, 2 );
    EXPECT_EQ( str.view(), "2.50" );
}

TEST( string, unsigned_to_string )
{
    String::Length<20> str;
    uint8_t value8 = 111;
    uint16_t value16 = 2222;
    uint32_t value32 = 700000;
    uint64_t value64 = 5000000000;

    str += value8;
    ASSERT_EQ( str.view(), "111" );

    str.clear();
    str += value16;
    ASSERT_EQ( str.view(), "2222" );

    str.clear();
    str += value32;
    ASSERT_EQ( str.view(), "700000" );

    str.clear();
    str += value64;
    ASSERT_EQ( str.view(), "5000000000" );

    str.clear();
    str += std::numeric_limits<uint64_t>::max();
    ASSERT_EQ( str.view(), "18446744073709551615" );

    str.clear();
    str += 42ul;
    ASSERT_EQ( str.view(), "42" );

    str.clear();
    str += 666u;
    ASSERT_EQ( str.view(), "666" );
}

TEST( string, signed_to_string )
{
    String::Length<20> str;
    int8_t value8 = -111;
    int16_t value16 = -2222;
    int32_t value32 = -700000;
    int64_t value64 = -5000000000;

    str += value8;
    ASSERT_EQ( str.view(), "-111" );

    str.clear();
    str += value16;
    ASSERT_EQ( str.view(), "-2222" );

    str.clear();
    str += value32;
    ASSERT_EQ( str.view(), "-700000" );

    str.clear();
    str += value64;
    ASSERT_EQ( str.view(), "-5000000000" );

    str.clear();
    str += std::numeric_limits<int64_t>::min();
    ASSERT_EQ( str.view(), "-9223372036854775808" );

    str.clear();
    str += -42l;
    ASSERT_EQ( str.view(), "-42" );

    str.clear();
    str += -666;
    ASSERT_EQ( str.view(), "-666" );
}

TEST( string, floating_to_string )
{
    String::Length<20> str;
    double value_d = -0.0012345678;
    float value_f = value_d;

    str += value_f;
    ASSERT_EQ( str.view(), "-0.001235" );

    str.clear();
    str += value_d;
    ASSERT_EQ( str.view(), "-0.001235" );
}

TEST( string, trim_right )
{
    String::Length<32> str;

    // Trailing spaces
    str = "test   ";
    str.trim_right();
    EXPECT_EQ( str.view(), "test" );

    // No trailing spaces
    str = "test";
    str.trim_right();
    EXPECT_EQ( str.view(), "test" );

    // All spaces
    str = "     ";
    str.trim_right();
    EXPECT_EQ( str.view(), "" );

    // Empty string
    str = "";
    str.trim_right();
    EXPECT_EQ( str.view(), "" );

    // Internal spaces (should not be trimmed)
    str = "te st  ";
    str.trim_right();
    EXPECT_EQ( str.view(), "te st" );
}
