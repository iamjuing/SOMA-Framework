// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/io/buffered_input_stream.h"
#include "soma/containers/ring_buffer.h"
#include "soma/io/i_input_stream.h"
#include "mock_observer.h"

using namespace SOMA;

namespace
{
    // Mock input stream for testing
    class Mock_input_stream: public I_input_stream
    {
    public:
        explicit Mock_input_stream( String_view name )
            : I_input_stream( name )
        {
        }

        MOCK_METHOD( Result_e, read, ( Buffer<uint8_t>& out_buffer ), ( override ) );
        MOCK_METHOD( void, cancel_read, (), ( override ) );

        // Helper to simulate async completion
        void complete_read()
        {
            notify();
        }

        I_input_stream::Result_e wait_for_completion( Buffer<uint8_t>& out_buffer )
        {
            mp_pending_buffer = &out_buffer;

            return I_input_stream::Result_e::wait;
        }

        void complete_read( Buffer_view<uint8_t> data )
        {
            ASSERT_NE( mp_pending_buffer, nullptr );
            ASSERT_EQ( mp_pending_buffer->insert( data ), data.size() );

            mp_pending_buffer = nullptr;

            notify();
        }

    private:
        Buffer<uint8_t>* mp_pending_buffer = nullptr;
    };
}

TEST( buffered_input_stream, read_ready_when_data_available )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    // Constructor calls start_reading() which attempts to read
    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3, 4, 5 };
                       size_t to_copy = buffer.space() < 5 ? buffer.space() : 5;
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < to_copy; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( to_copy );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::ready );
    ASSERT_GE( out_buffer.size(), 1u );
}

TEST( buffered_input_stream, read_wait_when_no_data_available )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    // Constructor calls start_reading() and gets wait
    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::wait );
}

TEST( buffered_input_stream, data_read_correctly )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3, 4, 5 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 5; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 5 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    buffered_stream.read( out_buffer );

    EXPECT_BUFFER_EQ( out_buffer.view_items(), 1, 2, 3, 4, 5 );
}

TEST( buffered_input_stream, multiple_reads_from_buffer )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
                       size_t to_copy = buffer.space() < 8 ? buffer.space() : 8;
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < to_copy; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( to_copy );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillRepeatedly( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data1[3] = {};
    Buffer<uint8_t> out_buffer1 { Array( out_data1 ) };

    ASSERT_EQ( buffered_stream.read( out_buffer1 ), I_input_stream::Result_e::ready );
    EXPECT_BUFFER_EQ( out_buffer1.view_items(), 1, 2, 3 );

    uint8_t out_data2[3] = {};
    Buffer<uint8_t> out_buffer2 { Array( out_data2 ) };

    ASSERT_EQ( buffered_stream.read( out_buffer2 ), I_input_stream::Result_e::ready );
    EXPECT_BUFFER_EQ( out_buffer2.view_items(), 4, 5, 6 );
}

TEST( buffered_input_stream, async_completion_provides_data )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    // First read returns wait, second provides data
    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( Return( I_input_stream::Result_e::wait ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 3; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 3 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::wait );

    // Simulate async completion - data goes to ring buffer, not directly to out_buffer
    mock_stream.complete_read();

    // Ring buffer should now have data
    EXPECT_GE( ring_buffer.size(), 1u );
}

TEST( buffered_input_stream, notifies_upper_layer_when_data_arrives )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( Return( I_input_stream::Result_e::wait ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 3; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 3 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::wait );

    // Simulate async completion - should notify observer
    mock_stream.complete_read();
}

TEST( buffered_input_stream, bounded_copy_to_waiting_upper_layer_buffer )
{
    constexpr size_t max_async_read_completion_size = 64u;
    constexpr size_t received_data_size = 100u;

    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 128> ring_buffer;
    uint8_t received_data[received_data_size] = {};

    for( size_t i = 0; i < received_data_size; ++i )
    {
        received_data[i] = static_cast<uint8_t>( i );
    }

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( [&]( Buffer<uint8_t>& buffer ) {
                       return mock_stream.wait_for_completion( buffer );
                   } )
        .WillRepeatedly( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    uint8_t out_data[100] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    ASSERT_EQ( buffered_stream.read( out_buffer ), I_input_stream::Result_e::wait );

    mock_stream.complete_read( Buffer_view<uint8_t>( Array( received_data ) ) );

    ASSERT_EQ( out_buffer.size(), max_async_read_completion_size );
    ASSERT_EQ( ring_buffer.size(), received_data_size - max_async_read_completion_size );
    ASSERT_MEM_EQ( out_data, received_data, max_async_read_completion_size );

    uint8_t remaining_data[received_data_size] = {};
    Buffer<uint8_t> remaining_buffer { Array( remaining_data ) };

    ASSERT_EQ( buffered_stream.read( remaining_buffer ), I_input_stream::Result_e::ready );
    ASSERT_EQ( remaining_buffer.size(), received_data_size - max_async_read_completion_size );
    ASSERT_MEM_EQ( remaining_data,
                   &received_data[max_async_read_completion_size],
                   received_data_size - max_async_read_completion_size );
}

TEST( buffered_input_stream, continues_reading_after_async_completion )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    // Initial read waits, then completes, then continues reading
    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( Return( I_input_stream::Result_e::wait ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 3; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 3 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    buffered_stream.read( out_buffer );

    // Async completion triggers continue reading
    mock_stream.complete_read();
}

TEST( buffered_input_stream, buffers_data_when_no_reader_waiting )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3, 4, 5 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 5; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 5 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    // Constructor fills buffer
    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    // Data is already buffered, read returns immediately
    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::ready );
    EXPECT_BUFFER_EQ( out_buffer.view_items(), 1, 2, 3, 4, 5 );
}

TEST( buffered_input_stream, read_triggers_refill )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 3; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 3 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    // Read data, which should trigger start_reading() to refill
    buffered_stream.read( out_buffer );
}

TEST( buffered_input_stream, handles_empty_read )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       buffer.place( 0 ); // No data available

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data[10] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::wait );
}

TEST( buffered_input_stream, fills_buffer_completely_on_construction )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    // start_reading calls read once and fills the entire buffer, no space left so loop exits
    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t* p_buffer = buffer.get_space().storage();
                       size_t space = buffer.space();
                       for( size_t i = 0; i < space; ++i )
                       {
                           p_buffer[i] = static_cast<uint8_t>( i + 1 );
                       }
                       buffer.place( space );

                       return I_input_stream::Result_e::ready;
                   } );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    // Buffer should be full
    ASSERT_EQ( ring_buffer.space(), 0u );
}

TEST( buffered_input_stream, partial_buffer_read )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 10> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
                       size_t to_copy = buffer.space() < 8 ? buffer.space() : 8;
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < to_copy; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( to_copy );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillRepeatedly( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    // Read only 2 bytes
    uint8_t out_data[2] = {};
    Buffer<uint8_t> out_buffer { Array( out_data ) };

    I_input_stream::Result_e result = buffered_stream.read( out_buffer );

    ASSERT_EQ( result, I_input_stream::Result_e::ready );
    ASSERT_EQ( out_buffer.size(), 2u );
    EXPECT_BUFFER_EQ( out_buffer.view_items(), 1, 2 );
}

TEST( buffered_input_stream, async_read_with_no_waiting_caller )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 5> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( Return( I_input_stream::Result_e::wait ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3 };
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < 3; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( 3 );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );
    Mock_observer observer( buffered_stream );

    // No read called, so no upper layer waiting
    EXPECT_CALL( observer, notified() )
        .Times( 0 );

    // Async completion without waiting caller - data buffered, no notification
    mock_stream.complete_read();
}

TEST( buffered_input_stream, consecutive_ready_reads )
{
    Mock_input_stream mock_stream( "mock" );
    Ring_buffer<uint8_t, 20> ring_buffer;

    // **** EXPECTATIONS *********************************************************************************************

    EXPECT_CALL( mock_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& buffer ) {
                       uint8_t data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
                       size_t to_copy = buffer.space() < 12 ? buffer.space() : 12;
                       uint8_t* p_buffer = buffer.get_space().storage();
                       for( size_t i = 0; i < to_copy; ++i )
                       {
                           p_buffer[i] = data[i];
                       }
                       buffer.place( to_copy );

                       return I_input_stream::Result_e::ready;
                   } )
        .WillRepeatedly( Return( I_input_stream::Result_e::wait ) );

    // **** TEST SEQUENCE ********************************************************************************************

    Buffered_input_stream buffered_stream( "buffered", mock_stream, ring_buffer );

    uint8_t out_data1[4] = {};
    Buffer<uint8_t> out_buffer1 { Array( out_data1 ) };

    ASSERT_EQ( buffered_stream.read( out_buffer1 ), I_input_stream::Result_e::ready );

    uint8_t out_data2[4] = {};
    Buffer<uint8_t> out_buffer2 { Array( out_data2 ) };

    ASSERT_EQ( buffered_stream.read( out_buffer2 ), I_input_stream::Result_e::ready );

    uint8_t out_data3[4] = {};
    Buffer<uint8_t> out_buffer3 { Array( out_data3 ) };

    ASSERT_EQ( buffered_stream.read( out_buffer3 ), I_input_stream::Result_e::ready );
}
