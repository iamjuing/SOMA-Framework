// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "mock_observer.h"
#include "soma/containers/queue_spsc.h"

using namespace SOMA;

TEST( queue_spsc, push_item )
{
    uint8_t item = 0;
    Queue_SPSC<uint8_t, 5> queue;

    EXPECT_TRUE( queue.is_empty() );
    EXPECT_EQ( queue.size(), 0 );
    EXPECT_EQ( queue.capacity(), 5 );

    EXPECT_TRUE( queue.push( 1 ) );
    EXPECT_TRUE( queue.push( 2 ) );
    EXPECT_TRUE( queue.push( 3 ) );
    EXPECT_TRUE( queue.push( 4 ) );
    EXPECT_TRUE( queue.push( 5 ) );
    EXPECT_FALSE( queue.push( 6 ) );

    EXPECT_EQ( queue.capacity(), 5 );

    EXPECT_FALSE( queue.is_empty() );
    EXPECT_EQ( queue.size(), 5 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 1 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 2 );

    EXPECT_EQ( queue.size(), 3 );

    EXPECT_TRUE( queue.push( 7 ) );
    EXPECT_TRUE( queue.push( 8 ) );
    EXPECT_FALSE( queue.push( 9 ) );

    EXPECT_EQ( queue.size(), 5 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 3 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 4 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 5 );

    EXPECT_EQ( queue.size(), 2 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 7 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 8 );

    EXPECT_FALSE( queue.pop( item ) );

    EXPECT_TRUE( queue.is_empty() );
    EXPECT_EQ( queue.size(), 0 );
}

TEST( queue_spsc, push_buffer )
{
    uint8_t item = 0;
    uint8_t data[] = { 1, 2, 3 };
    Buffer_view<uint8_t> buffer { Array( data ) };

    Queue_SPSC<uint8_t, 5> queue;

    EXPECT_EQ( queue.push( buffer ), 3 );

    EXPECT_EQ( queue.size(), 3 );

    EXPECT_TRUE( queue.pop( item ) );

    EXPECT_EQ( queue.push( buffer ), 3 );

    EXPECT_EQ( queue.size(), 5 );

    EXPECT_TRUE( queue.pop( item ) );

    EXPECT_EQ( queue.push( buffer ), 1 );

    EXPECT_EQ( queue.size(), 5 );
}

TEST( queue_spsc, push_observe )
{
    Queue_SPSC<uint8_t, 5> queue;
    Mock_observer observer( queue.signal_items_available() );

    // **** EXPECTATIONS *********************************************************************************************************

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    // **** TEST SEQUENCE ********************************************************************************************************

    queue.push( 1 );
    queue.push( 2 );
    queue.push( 3 );
    queue.push( 4 );
    queue.push( 5 );
    queue.push( 6 );
}

TEST( queue_spsc, mixed )
{
    Queue_SPSC<uint8_t, 5> queue;
    Mock_observer observer( queue.signal_items_available() );

    // **** EXPECTATIONS *********************************************************************************************************

    EXPECT_CALL( observer, notified() )
        .Times( 2 );

    // **** TEST SEQUENCE ********************************************************************************************************
    uint8_t item = 0;

    EXPECT_TRUE( queue.push( 1 ) );
    EXPECT_TRUE( queue.push( 2 ) );
    EXPECT_TRUE( queue.push( 3 ) );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 1 );
    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 2 );

    EXPECT_TRUE( queue.push( 4 ) );
    EXPECT_TRUE( queue.push( 5 ) );
    EXPECT_TRUE( queue.push( 6 ) );

    Buffer<uint8_t, 2> buffer;

    queue.pop( buffer );
    EXPECT_EQ( buffer.size(), 2 );
    EXPECT_BUFFER_EQ( buffer.view_items(), 3, 4 );

    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 5 );
    EXPECT_TRUE( queue.pop( item ) );
    EXPECT_EQ( item, 6 );

    EXPECT_FALSE( queue.pop( item ) );

    EXPECT_TRUE( queue.push( 1 ) );
    EXPECT_TRUE( queue.push( 2 ) );
    EXPECT_TRUE( queue.push( 3 ) );
    EXPECT_TRUE( queue.push( 4 ) );
    EXPECT_TRUE( queue.push( 5 ) );
    EXPECT_FALSE( queue.push( 6 ) );
    EXPECT_FALSE( queue.push( 7 ) );
}

TEST( queue_spsc, raw_access )
{
    Queue_SPSC<uint8_t, 5> queue;

    EXPECT_TRUE( queue.is_empty() );

    auto free_buffer = queue.get_space();

    free_buffer.insert( 1 );
    free_buffer.insert( 2 );
    free_buffer.insert( 3 );
    free_buffer.insert( 4 );
    free_buffer.insert( 5 );

    queue.place( 5 );
    EXPECT_EQ( queue.size(), 5 );
    EXPECT_FALSE( queue.is_empty() );
    EXPECT_TRUE( queue.is_full() );

    EXPECT_EQ( queue.get_space().space(), 0 );

    EXPECT_BUFFER_EQ( queue.view_items(), 1, 2, 3, 4, 5 );
}
