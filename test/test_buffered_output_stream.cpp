// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/io/buffered_output_stream.h"
#include "soma/containers/ring_buffer.h"
#include "soma/io/i_output_stream.h"
#include "mock_observer.h"

using namespace SOMA;

namespace
{
    // Mock output stream for testing
    class Mock_output_stream: public I_output_stream
    {
    public:
        explicit Mock_output_stream( String_view name )
            : I_output_stream( name )
        {
        }

        MOCK_METHOD( Result_e, write, ( Buffer_view<uint8_t> data ), ( override ) );

        // Helper to simulate async completion - calls notify() which is protected
        void complete_write()
        {
            // Trigger the observers to simulate async completion
            notify();
        }
    };
}

TEST( buffered_output_stream, write_ready_when_underlying_stream_ready )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::ready ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::ready );
}

TEST( buffered_output_stream, write_wait_when_underlying_stream_waits )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::ready );
}

TEST( buffered_output_stream, write_unavailable_when_underlying_stream_unavailable )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::unavailable ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::unavailable );
}

TEST( buffered_output_stream, data_buffered_correctly )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( [&]( Buffer_view<uint8_t> data ) {
                       EXPECT_BUFFER_EQ( data, 1, 2, 3, 4, 5 );

                       return I_output_stream::Result_e::ready;
                   } );

    // **** TEST SEQUENCE ********************************************************************************************

    buffered_stream.write( buffer );
}

TEST( buffered_output_stream, multiple_writes_when_buffer_has_space )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data1[] = { 1, 2, 3 };
    uint8_t data2[] = { 4, 5, 6 };
    Buffer_view<uint8_t> buffer1 { Array( data1 ) };
    Buffer_view<uint8_t> buffer2 { Array( data2 ) };

    // **** EXPECTATIONS *********************************************************************************************

    // First write starts async operation, second write is buffered (no new underlying write)
    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result1 = buffered_stream.write( buffer1 );

    ASSERT_EQ( result1, I_output_stream::Result_e::ready );

    // Second write while async in progress - data is buffered
    I_output_stream::Result_e result2 = buffered_stream.write( buffer2 );

    ASSERT_EQ( result2, I_output_stream::Result_e::ready );
}

TEST( buffered_output_stream, async_completion_continues_writing )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data1[] = { 1, 2, 3 };
    uint8_t data2[] = { 4, 5, 6 };
    Buffer_view<uint8_t> buffer1 { Array( data1 ) };
    Buffer_view<uint8_t> buffer2 { Array( data2 ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) )
        .WillOnce( Return( I_output_stream::Result_e::ready ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result1 = buffered_stream.write( buffer1 );

    ASSERT_EQ( result1, I_output_stream::Result_e::ready );

    I_output_stream::Result_e result2 = buffered_stream.write( buffer2 );

    ASSERT_EQ( result2, I_output_stream::Result_e::ready );

    // Simulate async completion
    mock_stream.complete_write();
}

TEST( buffered_output_stream, notifies_upper_layer_when_transfer_completes )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    // First write of buffered data (10 bytes), then async completion writes remaining (1 byte)
    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) )
        .WillOnce( Return( I_output_stream::Result_e::ready ) );

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::wait );

    // Simulate async completion - writes remaining data and notifies
    mock_stream.complete_write();
}

TEST( buffered_output_stream, buffer_overflow_returns_wait )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 5> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::wait );
}

TEST( buffered_output_stream, processes_remaining_buffer_after_completion )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 5> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) )
        .WillOnce( Return( I_output_stream::Result_e::ready ) );

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::wait );

    // Simulate async completion - should trigger write of remaining data
    mock_stream.complete_write();
}

TEST( buffered_output_stream, clears_buffer_on_unavailable )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    // First write returns unavailable (clears buffer), no subsequent underlying write
    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::unavailable ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::unavailable );

    // Buffer is cleared - verify ring buffer is empty
    ASSERT_TRUE( ring_buffer.is_empty() );
}

TEST( buffered_output_stream, handles_partial_buffer_fill )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data1[] = { 1, 2 };
    uint8_t data2[] = { 3, 4 };
    Buffer_view<uint8_t> buffer1 { Array( data1 ) };
    Buffer_view<uint8_t> buffer2 { Array( data2 ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( [&]( Buffer_view<uint8_t> data ) {
                       EXPECT_BUFFER_EQ( data, 1, 2 );

                       return I_output_stream::Result_e::wait;
                   } )
        .WillOnce( [&]( Buffer_view<uint8_t> data ) {
                       EXPECT_BUFFER_EQ( data, 3, 4 );

                       return I_output_stream::Result_e::ready;
                   } );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result1 = buffered_stream.write( buffer1 );

    ASSERT_EQ( result1, I_output_stream::Result_e::ready );

    I_output_stream::Result_e result2 = buffered_stream.write( buffer2 );

    ASSERT_EQ( result2, I_output_stream::Result_e::ready );

    // Simulate async completion - writes buffered data (3,4)
    mock_stream.complete_write();
}

TEST( buffered_output_stream, write_during_async_operation_buffers_data )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data1[] = { 1, 2, 3 };
    uint8_t data2[] = { 4, 5 };
    Buffer_view<uint8_t> buffer1 { Array( data1 ) };
    Buffer_view<uint8_t> buffer2 { Array( data2 ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) )
        .WillOnce( [&]( Buffer_view<uint8_t> data ) {
                       EXPECT_BUFFER_EQ( data, 4, 5 );

                       return I_output_stream::Result_e::ready;
                   } );

    // **** TEST SEQUENCE ********************************************************************************************

    // First write starts async operation
    I_output_stream::Result_e result1 = buffered_stream.write( buffer1 );

    ASSERT_EQ( result1, I_output_stream::Result_e::ready );

    // Second write during async operation should buffer
    I_output_stream::Result_e result2 = buffered_stream.write( buffer2 );

    ASSERT_EQ( result2, I_output_stream::Result_e::ready );

    // Simulate async completion - buffered data should be written
    mock_stream.complete_write();
}

TEST( buffered_output_stream, consecutive_ready_writes )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t data1[] = { 1, 2, 3 };
    Buffer_view<uint8_t> buffer1 { Array( data1 ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::ready ) );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer1 );

    ASSERT_EQ( result, I_output_stream::Result_e::ready );
}

TEST( buffered_output_stream, async_unavailable_notifies_waiting_caller )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 5> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) )
        .WillOnce( Return( I_output_stream::Result_e::unavailable ) );

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::wait );

    // Simulate async completion with unavailable - should notify caller
    mock_stream.complete_write();
}

TEST( buffered_output_stream, empty_buffer_after_async_completion_no_upper_layer_wait )
{
    Mock_output_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;
    Buffered_output_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    uint8_t data[] = { 1, 2, 3 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, write( _ ) )
        .WillOnce( Return( I_output_stream::Result_e::wait ) );

    EXPECT_CALL( observer, notified() )
        .Times( 0 );

    // **** TEST SEQUENCE ********************************************************************************************

    I_output_stream::Result_e result = buffered_stream.write( buffer );

    ASSERT_EQ( result, I_output_stream::Result_e::ready );

    // Simulate async completion - no upper layer waiting, no notification
    mock_stream.complete_write();
}
