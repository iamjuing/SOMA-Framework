// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include <limits>
#include <type_traits>

#include "soma/text/text_output.h"
#include "soma/io/i_blocking_output_stream.h"
#include "soma/text/string.h"

using namespace SOMA;

class Mock_blocking_output_stream: public I_blocking_output_stream
{
public:
    Mock_blocking_output_stream( uint8_t* p_buffer,
                                 size_t size )
        : mp_buffer( p_buffer ),
          m_size( size )
    {
        // intentionally empty
    }

    bool write( Buffer_view<uint8_t> data ) override
    {
        size_t to_copy = m_size - m_position_idx;

        to_copy = std::min( to_copy, data.size() );

        memcpy( &mp_buffer[m_position_idx], data.array().storage(), to_copy );

        m_position_idx += to_copy;

        return to_copy == data.size();
    }

    void clear( void )
    {
        m_position_idx = 0;
    }

private:
    uint8_t* mp_buffer = nullptr;
    size_t m_size;
    size_t m_position_idx = 0;
};

#define C_STRING_PAIR( str ) str, strlen( str )

TEST( text_output, append )
{
    uint8_t buffer[50];
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    String::Length<40> test_string( " test string   " );
    int32_t integer_number = -42;
    float floating_point_number = 0.123456f;

    text_output << test_string;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( " test string   " ) ), 0 );

    text_output << integer_number;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( " test string   -42" ) ), 0 );

    text_output << floating_point_number;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( " test string   -420.123456" ) ), 0 );
}


TEST( text_output, multiple_values )
{
    uint8_t buffer[50];
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    String_view test_string( "test string" );
    int32_t integer_number = -42;
    float floating_point_number = 0.123456f;

    text_output << test_string << integer_number << floating_point_number;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "test string-420.123456" ) ), 0 );
}

TEST( text_output, manipulator_left )
{
    uint8_t buffer[50];
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    text_output << min_width( 10 ) << left << "left";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "left      " ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 10 ) << "right";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "     right" ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 3 ) << left << "left";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "left" ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 3 ) << "right";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "right" ) ), 0 );
}

TEST( text_output, manipulator_hex )
{
    uint8_t buffer[50];
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    text_output << hex << 666;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "29A" ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 10 ) << hex << 666;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "       29A" ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 10 ) << left << hex << 666;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "29A       " ) ), 0 );

    mock_stream.clear();
    text_output << hex << 12.12f;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "12.12" ) ), 0 );
}

TEST( text_output, prints_64_bit_decimal_numbers_without_printf_long_long_support )
{
    uint8_t buffer[80] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    text_output << std::numeric_limits<uint64_t>::max();
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "18446744073709551615" ) ), 0 );

    mock_stream.clear();
    memset( buffer, 0, sizeof( buffer ) );

    text_output << std::numeric_limits<int64_t>::min();
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "-9223372036854775808" ) ), 0 );
}

TEST( text_output, manipulator_bytes_preserves_unsigned_argument_type )
{
    static_assert( std::is_same_v<decltype( bytes( uint16_t { 0 }, 1 ) ), bytes<uint16_t>> );
    static_assert( std::is_same_v<decltype( bytes( uint32_t { 0 }, 1 ) ), bytes<uint32_t>> );
    static_assert( std::is_same_v<decltype( bytes( uint64_t { 0 }, 1 ) ), bytes<uint64_t>> );

    uint8_t buffer[100] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    text_output << bytes( uint16_t { 512 }, 1 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "512 B" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    text_output << bytes( uint32_t { 1536 }, 1 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "1.5 kB" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    const uint64_t five_gib = 5ull * 1024ull * 1024ull * 1024ull;

    text_output << bytes( five_gib, 1 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "5120.0 MB" ) ), 0 );
}

TEST( text_output, manipulator_set_fill )
{
    uint8_t buffer[50];
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    text_output << min_width( 10 ) << fill_with( '*' ) << left << "left";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "left******" ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 10 ) << fill_with( '*' ) << "right";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "*****right" ) ), 0 );

    mock_stream.clear();
    text_output << min_width( 8 ) << fill_with( '0' ) << hex << 666;
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "0000029A" ) ), 0 );
}

TEST( text_output, buffering_basic )
{
    uint8_t buffer[50] = { 0 }; // Initialize to zero
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );

    Buffer<char, 20> line_buffer;
    Text_output text_output( mock_stream, line_buffer );

    // Data should be buffered, not written to stream yet
    text_output << "test";
    ASSERT_EQ( buffer[0], 0 ); // Stream buffer should be empty

    // Manual flush should write to stream
    text_output.flush();
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "test" ) ), 0 );
}

TEST( text_output, buffering_auto_flush_on_newline )
{
    uint8_t buffer[50] = { 0 }; // Initialize to zero
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );

    Buffer<char, 20> line_buffer;
    Text_output text_output( mock_stream, line_buffer );

    // Add data without newline - should be buffered
    text_output << "line1";
    ASSERT_EQ( buffer[0], 0 ); // Stream buffer should be empty

    // Add newline - should auto-flush
    text_output << "\n";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "line1\n" ) ), 0 );

    // Clear buffer for next test
    mock_stream.clear();
    memset( buffer, 0, sizeof( buffer ) );

    // Add more data after newline - should be buffered again
    text_output << "line2";
    ASSERT_EQ( buffer[0], 0 ); // Stream buffer should be empty again

    // Manual flush
    text_output.flush();
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "line2" ) ), 0 );
}

TEST( text_output, buffering_no_buffer_compatibility )
{
    uint8_t buffer[50] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream ); // No buffer provided

    // Without buffer, should write directly to stream
    text_output << "test";
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "test" ) ), 0 );

    // flush() should be safe to call even without buffer
    text_output.flush();
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "test" ) ), 0 );
}

TEST( text_output, buffering_flush_on_destruction )
{
    uint8_t buffer[50] = { 0 }; // Initialize to zero
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );

    Buffer<char, 20> line_buffer;

    {
        // Create Text_output in a scope that will be destroyed
        Text_output text_output( mock_stream, line_buffer );

        // Add data without newline - should be buffered
        text_output << "buffered_data";

        // Verify data is still buffered (not written to stream yet)
        ASSERT_EQ( buffer[0], 0 ); // Stream buffer should be empty

        // Text_output destructor will be called here when exiting scope
        // This should automatically flush the line buffer
    }

    // After destruction, buffered data should be written to stream
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "buffered_data" ) ), 0 );
}

TEST( text_output, fraction_unsigned )
{
    uint8_t buffer[100] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // 23 / 4188 = 0.00549 -> rounded to 2 decimals = 0.01
    text_output << fraction( 23u, 4188u, 2 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "0.01" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // 2043 / 503902 ~= 0.00405 -> 0.00
    text_output << fraction( 2043u, 503902u, 2 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "0.00" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // 1556 / 23 = 67.65217 -> 2 decimals = 67.65
    text_output << fraction( 1556u, 23u, 2 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "67.65" ) ), 0 );
}

TEST( text_output, fraction_signed_basic )
{
    uint8_t buffer[100] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Positive signed with many decimals
    text_output << fraction( 1556, 23, 5 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "67.65217" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // Rounding with zero decimals (1556/23 = 67.65217 -> 68)
    text_output << fraction( 1556, 23, 0 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "68" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // Negative numerator
    text_output << fraction( -1556, 23, 2 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "-67.65" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // Negative numerator rounding to integer
    text_output << fraction( -1556, 23, 0 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "-68" ) ), 0 );
}

TEST( text_output, fraction_denominator_sign_variants )
{
    uint8_t buffer[100] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Both negative -> positive result
    text_output << fraction( -1556, -23, 1 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "67.7" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // Denominator negative -> negative result
    text_output << fraction( 1556, -23, 1 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "-67.7" ) ), 0 );
}

TEST( text_output, fraction_rounding_edge_cases )
{
    uint8_t buffer[100] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // 5/2 = 2.5 -> 3 decimals -> 2.500
    text_output << fraction( 5, 2, 3 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "2.500" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // 2 decimals -> 2.50
    text_output << fraction( 5, 2, 2 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "2.50" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // Zero decimals rounding up (2.5 -> 3)
    text_output << fraction( 5, 2, 0 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "3" ) ), 0 );
    mock_stream.clear(); memset( buffer, 0, sizeof( buffer ) );

    // Negative rounding away from zero (-2.5 -> -3)
    text_output << fraction( -5, 2, 0 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "-3" ) ), 0 );
}

TEST( text_output, memory_dump_basic )
{
    uint8_t buffer[200] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Test with uint8_t array
    Array<uint8_t, 8> data = { 0, 1, 2, 3, 4, 5, 6, 7 };

    text_output << memory_dump( data, 4 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "         0:   0   1   2   3\r\n         4:   4   5   6   7\r\n" ) ), 0 );
}

TEST( text_output, memory_dump_hex )
{
    uint8_t buffer[200] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Test with hex formatting
    Array<uint8_t, 8> data = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };

    text_output << hex << min_width( 2 ) << fill_with( '0' ) << memory_dump( data, 4 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "0x0000000000: 00 11 22 33\r\n0x0000000004: 44 55 66 77\r\n" ) ), 0 );
}

TEST( text_output, memory_dump_uint16 )
{
    uint8_t buffer[200] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Test with uint16_t array
    Array<uint16_t, 6> data = { 100, 200, 300, 400, 500, 600 };

    text_output << memory_dump( data, 3 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "         0:   100   200   300\r\n         3:   400   500   600\r\n" ) ), 0 );
}

TEST( text_output, memory_dump_single_column )
{
    uint8_t buffer[200] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Test with single column
    Array<uint32_t, 3> data = { 1000, 2000, 3000 };

    text_output << memory_dump( data, 1 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "         0:       1000\r\n         1:       2000\r\n         2:       3000\r\n" ) ), 0 );
}

TEST( text_output, memory_dump_with_initial_index )
{
    uint8_t buffer[200] = { 0 };
    Mock_blocking_output_stream mock_stream( buffer, sizeof( buffer ) );
    Text_output text_output( mock_stream );

    // Test with initial index
    Array<uint8_t, 8> data = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11 };

    text_output << hex << min_width( 2 ) << fill_with( '0' ) << memory_dump( data, 4, 0x1000 );
    ASSERT_EQ( memcmp( reinterpret_cast<char*>( buffer ), C_STRING_PAIR( "0x0000001000: AA BB CC DD\r\n0x0000001004: EE FF 00 11\r\n" ) ), 0 );
}
