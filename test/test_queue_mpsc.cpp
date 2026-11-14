// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "mock_observer.h"
#include "soma/containers/queue_mpsc.h"

using namespace SOMA;

TEST( queue_mpsc, push_item )
{
    Queue_MPSC<uint8_t, 5> queue;

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

    EXPECT_BUFFER_EQ( queue.view_items(), 1, 2, 3, 4, 5 );

    queue.displace( 1 );
    EXPECT_BUFFER_EQ( queue.view_items(), 2, 3, 4, 5 );

    queue.displace( 1 );
    EXPECT_EQ( queue.size(), 3 );

    EXPECT_TRUE( queue.push( 7 ) );
    EXPECT_TRUE( queue.push( 8 ) );
    EXPECT_FALSE( queue.push( 9 ) );
    EXPECT_EQ( queue.size(), 5 );
    EXPECT_BUFFER_EQ( queue.view_items(), 3, 4, 5 );

    queue.displace( 1 );
    EXPECT_BUFFER_EQ( queue.view_items(), 4, 5 );

    queue.displace( 1 );
    EXPECT_BUFFER_EQ( queue.view_items(), 5 );

    queue.displace( 1 );
    EXPECT_EQ( queue.size(), 2 );
    EXPECT_BUFFER_EQ( queue.view_items(), 7, 8 );

    queue.displace( 1 );
    queue.displace( 1 );

    EXPECT_TRUE( queue.is_empty() );
    EXPECT_EQ( queue.size(), 0 );
    EXPECT_EQ( queue.view_items().size(), 0 );
}

TEST( queue_mpsc, push_buffer )
{
    uint8_t storage[] = { 1, 2, 3 };
    Buffer_view<uint8_t> data { Array( storage ) };

    Queue_MPSC<uint8_t, 5> queue;

    EXPECT_EQ( queue.push( data ), 3 );
    EXPECT_EQ( queue.size(), 3 );
    EXPECT_BUFFER_EQ( queue.view_items(), 1, 2, 3 );

    queue.displace( 1 );
    EXPECT_EQ( queue.size(), 2 );

    EXPECT_EQ( queue.push( data ), 3 );
    EXPECT_EQ( queue.size(), 5 );
    EXPECT_BUFFER_EQ( queue.view_items(), 2, 3, 1, 2 );

    queue.displace( 1 );
    EXPECT_EQ( queue.size(), 4 );
    EXPECT_BUFFER_EQ( queue.view_items(), 3, 1, 2 );

    EXPECT_EQ( queue.push( data ), 1 );
    EXPECT_EQ( queue.size(), 5 );

    queue.displace( 4 );
    EXPECT_EQ( queue.size(), 1 );
    EXPECT_BUFFER_EQ( queue.view_items(), 1 );
}

TEST( queue_mpsc, push_observe )
{
    Queue_MPSC<uint8_t, 5> queue;
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

TEST( queue_mpsc, mixed )
{
    Queue_MPSC<uint8_t, 5> queue;
    Mock_observer observer( queue.signal_items_available() );

    // **** EXPECTATIONS *********************************************************************************************************

    EXPECT_CALL( observer, notified() )
        .Times( 2 );

    // **** TEST SEQUENCE ********************************************************************************************************
    uint8_t item = 0;

    EXPECT_TRUE( queue.push( 1 ) );
    EXPECT_TRUE( queue.push( 2 ) );
    EXPECT_TRUE( queue.push( 3 ) );

    EXPECT_EQ( queue.size(), 3 );
    EXPECT_BUFFER_EQ( queue.view_items(), 1, 2, 3 );

    queue.displace( 1 );
    EXPECT_EQ( queue.size(), 2 );

    EXPECT_BUFFER_EQ( queue.view_items(), 2, 3 );
    queue.displace( 1 );

    EXPECT_TRUE( queue.push( 4 ) );
    EXPECT_TRUE( queue.push( 5 ) );
    EXPECT_TRUE( queue.push( 6 ) );
    EXPECT_EQ( queue.size(), 4 );

    Buffer<uint8_t, 2> buffer;

    EXPECT_BUFFER_EQ( queue.view_items(), 3, 4, 5 );

    queue.displace( 2 );
    EXPECT_EQ( queue.size(), 2 );

    EXPECT_BUFFER_EQ( queue.view_items(), 5 );

    queue.displace( 1 );
    EXPECT_EQ( queue.size(), 1 );

    EXPECT_BUFFER_EQ( queue.view_items(), 6 );

    queue.displace( 1 );

    EXPECT_EQ( queue.view_items().size(), 0 );

    EXPECT_TRUE( queue.push( 1 ) );
    EXPECT_TRUE( queue.push( 2 ) );
    EXPECT_TRUE( queue.push( 3 ) );
    EXPECT_TRUE( queue.push( 4 ) );
    EXPECT_TRUE( queue.push( 5 ) );
    EXPECT_FALSE( queue.push( 6 ) );
    EXPECT_FALSE( queue.push( 7 ) );
}

TEST( queue_mpsc, push_isr )
{
    Queue_MPSC<uint8_t, 5> queue;
    Mock_observer observer( queue.signal_items_available() );

    // **** EXPECTATIONS *********************************************************************************************************

    EXPECT_CALL( observer, notified() )
        .Times( 1 );

    // **** TEST SEQUENCE ********************************************************************************************************

    EXPECT_TRUE( queue.push( 1 ) );

    set_ISR_context( true );
    EXPECT_TRUE( queue.push( 2 ) );
    set_ISR_context( false );
    EXPECT_EQ( queue.size(), 2 );

    EXPECT_TRUE( queue.push( 3 ) );
    EXPECT_EQ( queue.size(), 3 );

    queue.displace( 2 );

    uint8_t storage [] = { 4, 5, 6 };
    Buffer_view<uint8_t> data { Array( storage ) };

    set_ISR_context( true );
    EXPECT_TRUE( queue.push( data ) );
    set_ISR_context( false );
    EXPECT_EQ( queue.size(), 4 );
    EXPECT_BUFFER_EQ( queue.view_items(), 3, 4, 5 );

    queue.displace( 3 );
    EXPECT_EQ( queue.size(), 1 );

    EXPECT_BUFFER_EQ( queue.view_items(), 6 );
}
