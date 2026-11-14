// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include <type_traits>
#include <utility>

#include "soma/containers/buffer_view.h"

using namespace SOMA;

static_assert( std::is_same_v<decltype( std::declval<Buffer_view<uint8_t>>().array() ),
                              Array<const uint8_t>> );
static_assert( std::is_same_v<decltype( *std::declval<Buffer_view<uint8_t>>().begin() ),
                              const uint8_t&> );

TEST( buffer_view, access )
{
    uint8_t storage[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( storage ) };

    ASSERT_EQ( buffer.size(), 5 );

    size_t number = 1;

    for( auto& item: buffer )
    {
        ASSERT_EQ( item, number );
        ++number;
    }

    EXPECT_BUFFER_EQ( buffer, 1, 2, 3, 4, 5 );
}

TEST( buffer_view, raw_access )
{
    uint8_t storage[] = { 1, 2, 3, 4, 5 };
    Buffer_view<uint8_t> buffer { Array( storage ) };

    const uint8_t* p_buffer = buffer.array().storage();
    const size_t size = buffer.size();

    ASSERT_EQ( p_buffer, storage );
    ASSERT_EQ( size, 5 );

    uint8_t number = 1;

    for( size_t i = 0; i < size; ++i )
    {
        ASSERT_EQ( p_buffer[i], number );
        ++number;
    }
}

TEST( buffer_view, accepts_const_storage )
{
    const uint8_t storage[] = { 1, 2, 3, 4, 5 };
    Buffer_view buffer { Array( storage ) };

    static_assert( std::is_same_v<decltype( buffer ), Buffer_view<uint8_t>> );

    EXPECT_BUFFER_EQ( buffer, 1, 2, 3, 4, 5 );
}

TEST( buffer_view, sub_buffer )
{
    size_t number = 0;
    uint8_t storage[] = { 0, 1, 2, 3, 4 };
    Buffer_view<uint8_t> buffer { Array( storage ) };

    auto sub_buffer = buffer.sub_buffer( 0 );
    ASSERT_EQ( sub_buffer.size(), 5 );

    number = 0;
    for( auto& item: sub_buffer )
    {
        ASSERT_EQ( item, number );
        ++number;
    }

    sub_buffer = buffer.sub_buffer( 3 );
    ASSERT_EQ( sub_buffer.size(), 2 );

    number = 3;
    for( auto& item: sub_buffer )
    {
        ASSERT_EQ( item, number );
        ++number;
    }

    sub_buffer = buffer.sub_buffer( 1, 7 );
    ASSERT_EQ( sub_buffer.size(), 4 );

    number = 1;
    for( auto& item: sub_buffer )
    {
        ASSERT_EQ( item, number );
        ++number;
    }

    sub_buffer = buffer.sub_buffer( 2, 2 );
    ASSERT_EQ( sub_buffer.size(), 2 );

    number = 2;
    for( auto& item: sub_buffer )
    {
        ASSERT_EQ( item, number );
        ++number;
    }
}
