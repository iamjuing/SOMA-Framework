// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/containers/ring_buffer.h"
#include "soma/text/string_view.h"

using namespace SOMA;

TEST( ring_buffer, insert )
{
    size_t written;
    Ring_buffer<char, 10> rb;

    written = rb.insert( String_view( "111111" ) );
    ASSERT_EQ( written, 6 );
    ASSERT_EQ( rb.size(), 6 );

    written = rb.insert( String_view( "222222" ) );
    ASSERT_EQ( written, 4 );
    ASSERT_EQ( rb.size(), 10 );

    written = rb.insert( String_view( "333" ) );
    ASSERT_EQ( written, 0 );
    ASSERT_EQ( rb.size(), 10 );
}

TEST( ring_buffer, direct_insert )
{
    Buffer<char> buffer;
    size_t size;
    Ring_buffer<char, 10> rb;

    buffer = rb.get_space();
    ASSERT_EQ( buffer.space(), 10 );

    rb.place( 7 );
    ASSERT_EQ( rb.size(), 7 );
    ASSERT_EQ( rb.space(), 3 );

    buffer = rb.get_space();
    ASSERT_EQ( buffer.space(), 3 );

    rb.displace( 3 );
    ASSERT_EQ( rb.size(), 4 );
    ASSERT_EQ( rb.space(), 6 );

    buffer = rb.get_space();
    ASSERT_EQ( buffer.space(), 3 );

    rb.place( 2 );
    ASSERT_EQ( rb.size(), 6 );
    ASSERT_EQ( rb.space(), 4 );

    buffer = rb.get_space();
    ASSERT_EQ( buffer.space(), 1 );

    rb.place( 1 );
    ASSERT_EQ( rb.size(), 7 );
    ASSERT_EQ( rb.space(), 3 );

    buffer = rb.get_space();
    ASSERT_EQ( buffer.space(), 3 );

    rb.place( 3 );
    ASSERT_EQ( rb.size(), 10 );
    ASSERT_EQ( rb.space(), 0 );

    buffer = rb.get_space();
    ASSERT_EQ( buffer.space(), 0 );
}

TEST( ring_buffer, simple_remove )
{
    Ring_buffer<char, 10> rb;

    ASSERT_EQ( rb.insert( String_view( "111111" ) ), 6 );
    ASSERT_EQ( rb.size(), 6 );

    rb.displace( 2 );
    ASSERT_EQ( rb.size(), 4 );

    ASSERT_EQ( rb.insert( String_view( "222" ) ), 3 );
    ASSERT_EQ( rb.size(), 7 );

    ASSERT_EQ( rb.insert( String_view( "3" ) ), 1 );
    ASSERT_EQ( rb.size(), 8 );

    rb.displace( 5 );
    ASSERT_EQ( rb.size(), 3 );

    rb.displace( 3 );
    ASSERT_EQ( rb.size(), 0 );

    ASSERT_EQ( rb.insert( String_view( "44444" ) ), 5 ); // wraps around
    ASSERT_EQ( rb.size(), 5 );

    rb.displace( 5 );
    ASSERT_EQ( rb.size(), 0 );
}

TEST( ring_buffer, remove_data )
{
    size_t removed = 43;
    char data[100];
    Ring_buffer<char, 10> rb;

    rb.insert( String_view( "111111" ) );

    Buffer<char> out_buf( Array( data, 2 ) );

    removed = rb.remove( out_buf );
    ASSERT_EQ( removed, 2 );
    ASSERT_EQ( memcmp( data, "11", 2 ), 0 );

    rb.insert( String_view( "222" ) );

    Buffer<char> out_buf2( Array( data, 5 ) );

    removed = rb.remove( out_buf2 );
    ASSERT_EQ( removed, 5 );
    ASSERT_EQ( memcmp( data, "11112", 5 ), 0 );

    rb.insert( String_view( "3333333" ) ); // wraps around

    Buffer<char> out_buf3( Array( data, 6 ) );

    removed = rb.remove( out_buf3 );
    ASSERT_EQ( removed, 6 );
    ASSERT_EQ( memcmp( data, "223333", 6 ), 0 );

    Buffer<char> out_buf4( Array( data, 10 ) );

    removed = rb.remove( out_buf4 );
    ASSERT_EQ( removed, 3 );
    ASSERT_EQ( memcmp( data, "333", 3 ), 0 );
}

TEST( ring_buffer, bounded_remove_data )
{
    char data[10] = {};
    Ring_buffer<char, 10> rb;

    rb.insert( String_view( "abcdefgh" ) );
    rb.displace( 6 );
    rb.insert( String_view( "12345678" ) ); // wraps around and fills the buffer

    Buffer<char> out_buf { Array( data ) };

    size_t removed = rb.remove( out_buf, 7u );

    ASSERT_EQ( removed, 7u );
    ASSERT_EQ( out_buf.size(), 7u );
    ASSERT_EQ( rb.size(), 3u );
    ASSERT_MEM_EQ( data, "gh12345", 7 );

    char data2[10] = {};
    Buffer<char> out_buf2 { Array( data2 ) };

    removed = rb.remove( out_buf2, 10u );

    ASSERT_EQ( removed, 3u );
    ASSERT_EQ( out_buf2.size(), 3u );
    ASSERT_MEM_EQ( data2, "678", 3 );
}

TEST( ring_buffer, direct_access )
{
    Buffer_view<char> items;
    size_t removed;
    Ring_buffer<char, 10> rb;

    rb.insert( String_view( "11111111" ) );

    items = rb.view_items();
    ASSERT_EQ( items.size(), 8 );
    ASSERT_EQ( memcmp( items.array().storage(), "111111", 6 ), 0 );

    rb.displace( 6 );
    ASSERT_EQ( rb.space(), 8 );

    rb.insert( String_view( "22222" ) ); // wraps around
    rb.insert( String_view( "33333" ) ); // buffer full

    items = rb.view_items();
    ASSERT_EQ( items.size(), 4 );
    ASSERT_EQ( memcmp( items.array().storage(), "1122", items.size() ), 0 );

    rb.displace( 4 );
    ASSERT_EQ( rb.size(), 6 );

    items = rb.view_items();
    ASSERT_EQ( items.size(), 6 );
    ASSERT_EQ( memcmp( items.array().storage(), "222333", items.size() ), 0 );

    rb.displace( 6 );
    ASSERT_EQ( rb.size(), 0 );
}
