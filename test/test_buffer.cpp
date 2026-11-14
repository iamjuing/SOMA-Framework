// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include <type_traits>
#include <utility>

#include "soma/containers/buffer.h"

using namespace SOMA;

namespace
{
    struct Copy_checked_t
    {
        int value = 0;
        int* p_assignments = nullptr;

        Copy_checked_t& operator=( const Copy_checked_t& other )
        {
            value = other.value;
            p_assignments = other.p_assignments;
            ++*p_assignments;
            return *this;
        }
    };
}

TEST( buffer, insert_copies_elements_using_assignment )
{
    int assignments = 0;
    Copy_checked_t source[] = { { 42, &assignments } };
    Buffer<Copy_checked_t, 1> buffer;

    EXPECT_EQ( buffer.insert( Buffer_view( Array( source ) ) ), 1u );
    EXPECT_EQ( assignments, 1 );
    EXPECT_EQ( buffer.view_items()[0].value, 42 );
}

TEST( buffer, insert_empty_view_into_empty_storage )
{
    Buffer<uint8_t> buffer;
    EXPECT_EQ( buffer.insert( Buffer_view<uint8_t>() ), 0u );
}

TEST( buffer, insert )
{
    Buffer<uint8_t, 5> buffer;

    ASSERT_TRUE( buffer.insert( 1 ) );
    ASSERT_TRUE( buffer.insert( 2 ) );
    EXPECT_EQ( buffer.capacity(), 5 );

    ASSERT_EQ( buffer.size(), 2 );
    ASSERT_EQ( buffer.space(), 3 );

    uint8_t data[5] = { 3, 4, 5 };
    Buffer_view<uint8_t> data_buffer { Array( data ) };

    ASSERT_TRUE( buffer.insert( data_buffer ) );

    ASSERT_FALSE( buffer.insert( 5 ) );

    ASSERT_EQ( buffer.size(), 5 );
    ASSERT_EQ( buffer.space(), 0 );
    EXPECT_EQ( buffer.capacity(), 5 );

    size_t number = 1;

    for( auto& item: buffer.view_items() )
    {
        ASSERT_EQ( item, number );
        ++number;
    }
}

TEST( buffer, clear )
{
    Buffer<uint8_t, 5> buffer;

    ASSERT_TRUE( buffer.insert( 1 ) );
    ASSERT_TRUE( buffer.insert( 2 ) );

    ASSERT_EQ( buffer.size(), 2 );
    ASSERT_EQ( buffer.space(), 3 );

    size_t number = 1;

    for( auto& item: buffer.view_items() )
    {
        ASSERT_EQ( item, number );
        ++number;
    }

    buffer.clear();

    ASSERT_EQ( buffer.size(), 0 );
    ASSERT_EQ( buffer.space(), 5 );

    ASSERT_TRUE( buffer.insert( 3 ) );
    ASSERT_TRUE( buffer.insert( 4 ) );

    ASSERT_EQ( buffer.size(), 2 );
    ASSERT_EQ( buffer.space(), 3 );

    number = 3;

    for( auto& item: buffer.view_items() )
    {
        ASSERT_EQ( item, number );
        ++number;
    }
}

TEST( buffer, raw_access )
{
    Buffer<uint8_t, 5> buffer;
    uint8_t* p_buffer = buffer.get_space().storage();

    ASSERT_NE( p_buffer, nullptr );
    p_buffer[0] = 1;
    p_buffer[1] = 2;

    buffer.place( 2 );
    ASSERT_EQ( buffer.size(), 2 );
    ASSERT_EQ( buffer.space(), 3 );

    size_t number = 1;

    for( auto& item: buffer.view_items() )
    {
        ASSERT_EQ( item, number );
        ++number;
    }

    p_buffer = buffer.get_space().storage();

    ASSERT_NE( p_buffer, nullptr );
    p_buffer[0] = 3;
    p_buffer[1] = 4;
    p_buffer[2] = 5;

    buffer.place( 3 );
    ASSERT_EQ( buffer.size(), 5 );
    ASSERT_EQ( buffer.space(), 0 );

    number = 1;

    for( auto& item: buffer.view_items() )
    {
        ASSERT_EQ( item, number );
        ++number;
    }
}

TEST( buffer, converts_to_const_array )
{
    static_assert( std::is_convertible<Buffer<uint8_t>, Array<const uint8_t>>::value,
                   "Buffer<T> shall convert to Array<const T>" );

    Buffer<uint8_t, 5> buffer;

    ASSERT_TRUE( buffer.insert( 1 ) );
    ASSERT_TRUE( buffer.insert( 2 ) );

    Array<const uint8_t> array = buffer;

    ASSERT_EQ( array.size(), 2 );
    EXPECT_EQ( array[0], 1 );
    EXPECT_EQ( array[1], 2 );
}

TEST( buffer, owning_copy_and_move_keep_local_storage )
{
    static_assert( std::is_base_of_v<Buffer<uint8_t>, Buffer<uint8_t, 3>> );

    Buffer<uint8_t, 3> original { 1, 2 };
    Buffer<uint8_t, 3> copy { original };

    EXPECT_EQ( copy.size(), 2 );
    EXPECT_NE( static_cast<Array<uint8_t>>( copy ).storage(),
               static_cast<Array<uint8_t>>( original ).storage() );
    original.clear();
    original.insert( 7 );
    EXPECT_EQ( copy.view_items()[0], 1 );

    Buffer<uint8_t, 3> assigned;
    assigned = copy;
    EXPECT_EQ( assigned.size(), 2 );
    EXPECT_NE( static_cast<Array<uint8_t>>( assigned ).storage(),
               static_cast<Array<uint8_t>>( copy ).storage() );

    Buffer<uint8_t, 3> moved { std::move( copy ) };
    EXPECT_EQ( moved.size(), 2 );
    EXPECT_EQ( moved.view_items()[1], 2 );

    assigned = std::move( moved );
    EXPECT_EQ( assigned.size(), 2 );
    EXPECT_EQ( assigned.view_items()[0], 1 );
    EXPECT_NE( static_cast<Array<uint8_t>>( assigned ).storage(),
               static_cast<Array<uint8_t>>( moved ).storage() );
}
