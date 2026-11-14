// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/io/blocking_input_stream.h"
#include "soma/io/i_input_stream.h"

using namespace SOMA;

namespace
{
    class Mock_input_stream: public I_input_stream
    {
    public:
        Mock_input_stream()
            : I_input_stream( "mock" )
        {
            // intentionally empty
        }

        MOCK_METHOD( Result_e, read, ( Buffer<uint8_t>& out_buffer ), ( override ) );
        MOCK_METHOD( void, cancel_read, (), ( override ) );

        void complete_read( void )
        {
            notify();
        }
    };
}

TEST( blocking_input_stream, cancel_read_on_timeout )
{
    Mock_input_stream input_stream;
    Blocking_input_stream blocking_input_stream( input_stream );
    Buffer<uint8_t, 1> buffer;

    EXPECT_CALL( input_stream, read( _ ) )
        .WillOnce( Return( I_input_stream::Result_e::wait ) );
    EXPECT_CALL( input_stream, cancel_read() )
        .Times( 1 );

    blocking_input_stream.read( buffer, 1u );

    EXPECT_TRUE( buffer.is_empty() );
}

TEST( blocking_input_stream, does_not_cancel_ready_read )
{
    Mock_input_stream input_stream;
    Blocking_input_stream blocking_input_stream( input_stream );
    Buffer<uint8_t, 1> buffer;

    EXPECT_CALL( input_stream, read( _ ) )
        .WillOnce( []( Buffer<uint8_t>& out_buffer ) {
                       out_buffer.insert( 0x42u );

                       return I_input_stream::Result_e::ready;
                   } );
    EXPECT_CALL( input_stream, cancel_read() )
        .Times( 0 );

    blocking_input_stream.read( buffer, 1u );

    EXPECT_BUFFER_EQ( buffer.view_items(), 0x42 );
}

TEST( blocking_input_stream, drains_late_notification_before_next_read )
{
    Mock_input_stream input_stream;
    Blocking_input_stream blocking_input_stream( input_stream );
    Buffer<uint8_t, 1> buffer;

    EXPECT_CALL( input_stream, read( _ ) )
        .Times( 2 )
        .WillRepeatedly( Return( I_input_stream::Result_e::wait ) );
    EXPECT_CALL( input_stream, cancel_read() )
        .Times( 1 );

    blocking_input_stream.read( buffer, 1u );
    input_stream.complete_read();
    blocking_input_stream.read( buffer, 1u );
}
