// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include <type_traits>
#include <memory>

#include "soma/reverse_iterator.h"

using namespace SOMA;

TEST( array, deduces_single_view_as_an_owned_element )
{
    int storage[] { 10, 20 };
    Array<int> view { storage };
    Array single { view };
    Array multiple { view, view };
    Array<int> copied_view { view };

    static_assert( std::is_same_v<decltype( single ), Array<Array<int>, 1u>> );
    static_assert( std::is_same_v<decltype( multiple ), Array<Array<int>, 2u>> );
    ASSERT_EQ( single.size(), 1u );
    ASSERT_EQ( single[0].storage(), storage );
    ASSERT_EQ( copied_view.storage(), storage );
    Array<const Array<int>> regions = single;
    ASSERT_EQ( regions[0][1], 20 );
}

TEST( array, deduces_owned_arrays_as_views_of_their_storage )
{
    Array<uint8_t, 3u> first { 1u, 2u, 3u };
    Array<uint8_t, 5u> second { 4u, 5u, 6u, 7u, 8u };
    const Array<uint8_t, 2u> third { 9u, 10u };

    const Array single { first };
    const Array multiple { first, second, third };
    Array copied = first;

    static_assert( std::is_same_v<decltype( single ), const Array<Array<uint8_t>, 1u>> );
    static_assert( std::is_same_v<decltype( multiple ), const Array<Array<uint8_t>, 3u>> );
    static_assert( std::is_same_v<decltype( copied ), Array<uint8_t, 3u>> );

    ASSERT_EQ( single[0].storage(), first.storage() );
    ASSERT_EQ( multiple[0].storage(), first.storage() );
    ASSERT_EQ( multiple[1].storage(), second.storage() );
    ASSERT_EQ( multiple[2].storage(), third.storage() );
    ASSERT_NE( copied.storage(), first.storage() );

    Array<const Array<uint8_t>> regions = multiple;
    ASSERT_EQ( regions[1].size(), 5u );
}

TEST( array, owning_copy_and_move_keep_each_view_bound_to_its_storage )
{
    Array original { 10, 20, 30 };
    Array copied = original;
    Array moved = std::move( original );
    Array<int, 3> assigned;
    Array<int, 3> move_assigned;

    assigned = copied;
    move_assigned = std::move( moved );

    ASSERT_NE( copied.storage(), original.storage() );
    ASSERT_NE( moved.storage(), original.storage() );
    ASSERT_NE( assigned.storage(), copied.storage() );
    ASSERT_NE( move_assigned.storage(), moved.storage() );
    ASSERT_EQ( copied[1], 20 );
    ASSERT_EQ( move_assigned[2], 30 );
    copied[0] = 99;
    ASSERT_EQ( assigned[0], 10 );
    ASSERT_EQ( original[0], 10 );

    auto consume_view = []( Array<const int> values ) { return values[1]; };
    ASSERT_EQ( consume_view( assigned ), 20 );
    ASSERT_EQ( consume_view( copied ), 20 );
    static_assert( std::is_base_of_v<Array<int>, Array<int, 3>> );
}

TEST( array, deduction_distinguishes_external_storage_and_owned_elements )
{
    int storage[] { 10, 20, 30 };
    Array view { storage };
    Array bounded_view { storage, size_t { 2u } };
    Array owned { 10, 20, 30 };

    static_assert( std::is_same_v<decltype( view ), Array<int>> );
    static_assert( std::is_same_v<decltype( bounded_view ), Array<int>> );
    static_assert( std::is_same_v<decltype( owned ), Array<int, 3u>> );
    ASSERT_EQ( view.storage(), storage );
    ASSERT_EQ( bounded_view.size(), 2u );
    ASSERT_NE( owned.storage(), storage );
}

TEST( array, zero_extent_and_default_initialization )
{
    struct Non_default_constructible
    {
        Non_default_constructible() = delete;
    };

    Array<int, 0u> empty;
    Array<int, 0u> empty_copy { empty };
    Array<int, 0u> empty_move { std::move( empty_copy ) };
    Array<Non_default_constructible, 0u> empty_non_default;
    Array<int, 3u> owned;
    Array<int> empty_view;

    ASSERT_EQ( empty.size(), 0u );
    ASSERT_EQ( empty.begin(), empty.end() );
    ASSERT_EQ( empty_move.storage(), nullptr );
    ASSERT_EQ( empty_non_default.size(), 0u );
    ASSERT_EQ( empty_non_default.storage(), nullptr );
    ASSERT_EQ( empty_view.begin(), empty_view.end() );
    ASSERT_TRUE( empty == empty_view );
    ASSERT_EQ( owned[0], 0 );
    ASSERT_EQ( owned[2], 0 );
}

TEST( array, moves_owned_elements_without_rebinding_source_view )
{
    Array owned( std::make_unique<int>( 10 ), std::make_unique<int>( 20 ) );
    auto* source_storage = owned.storage();
    Array moved = std::move( owned );

    ASSERT_EQ( owned.storage(), source_storage );
    ASSERT_NE( moved.storage(), source_storage );
    ASSERT_EQ( owned[0], nullptr );
    ASSERT_EQ( *moved[0], 10 );
    ASSERT_EQ( *moved[1], 20 );
}

TEST( array, deduces_owned_element_type_and_count )
{
    const int first = 10;
    Array numbers { first, 20, 30 };
    Array single { 42 };

    static_assert( std::is_same_v<decltype( numbers ), Array<int, 3u>> );
    static_assert( std::is_same_v<decltype( single ), Array<int, 1u>> );

    Array<const int> view = numbers;
    ASSERT_EQ( view.size(), 3u );
    ASSERT_EQ( view[0], first );
    ASSERT_EQ( view[2], 30 );
    ASSERT_EQ( single[0], 42 );
}

TEST( array, variadic_constructor_initializes_non_default_constructible_elements )
{
    struct Item
    {
        explicit Item( int item_value ) : value( item_value ) {}
        int value;
    };

    Array items( Item { 10 }, Item { 20 } );
    Array copied = items;
    Array moved = std::move( items );

    static_assert( std::is_same_v<decltype( items ), Array<Item, 2u>> );
    static_assert( !std::is_constructible_v<Array<int, 2u>, int> );
    static_assert( !std::is_constructible_v<Array<int, 2u>, int, int, int> );
    ASSERT_EQ( items[0].value, 10 );
    ASSERT_EQ( items[1].value, 20 );
    ASSERT_NE( copied.storage(), items.storage() );
    ASSERT_NE( moved.storage(), items.storage() );
    ASSERT_EQ( copied[0].value, 10 );
    ASSERT_EQ( moved[1].value, 20 );
}

TEST( array, reading_items )
{
    uint8_t storage[] = { 1, 2, 3, 4, 5, 6 };
    Array<uint8_t> array( storage );

    ASSERT_EQ( array[0], 1 );
    ASSERT_EQ( array[3], 4 );
    ASSERT_EQ( array[5], 6 );
}

TEST( array, writing_items )
{
    char storage[10] = { '\0' };
    Array<char> char_array { storage };

    ASSERT_NE( char_array[0], 'g' );
    ASSERT_NE( char_array[5], 'a' );

    char_array[0] = 'g';
    char_array[5] = 'a';

    ASSERT_EQ( char_array[0], 'g' );
    ASSERT_EQ( char_array[5], 'a' );
}

TEST( array, iteration )
{
    Array<uint8_t, 5> numbers;

    uint8_t i = 0;
    for( auto& number: numbers )
    {
        number = i;

        ++i;
    }

    ASSERT_EQ( numbers[0], 0 );
    ASSERT_EQ( numbers[1], 1 );
    ASSERT_EQ( numbers[2], 2 );
    ASSERT_EQ( numbers[3], 3 );
    ASSERT_EQ( numbers[4], 4 );

    i = 10;
    for( auto& number: numbers )
    {
        number = i;

        ++i;
    }

    ASSERT_EQ( numbers[0], 10 );
    ASSERT_EQ( numbers[1], 11 );
    ASSERT_EQ( numbers[2], 12 );
    ASSERT_EQ( numbers[3], 13 );
    ASSERT_EQ( numbers[4], 14 );
}

TEST( array, reverse_iteration )
{
    Array<uint8_t, 5> numbers;

    uint8_t i = 0;
    for( auto& number: Reverse_iterator( numbers ) )
    {
        number = i;

        ++i;
    }

    ASSERT_EQ( numbers[0], 4 );
    ASSERT_EQ( numbers[1], 3 );
    ASSERT_EQ( numbers[2], 2 );
    ASSERT_EQ( numbers[3], 1 );
    ASSERT_EQ( numbers[4], 0 );
}

TEST( array, filling )
{
    Array<uint8_t, 5> numbers;

    numbers.fill( 42 );

    ASSERT_EQ( numbers[0], 42 );
    ASSERT_EQ( numbers[1], 42 );
    ASSERT_EQ( numbers[2], 42 );
    ASSERT_EQ( numbers[3], 42 );
    ASSERT_EQ( numbers[4], 42 );

    numbers.fill( 13 );

    ASSERT_EQ( numbers[0], 13 );
    ASSERT_EQ( numbers[1], 13 );
    ASSERT_EQ( numbers[2], 13 );
    ASSERT_EQ( numbers[3], 13 );
    ASSERT_EQ( numbers[4], 13 );
}

TEST( array, sub_array )
{
    uint8_t storage[] = { 10, 20, 30, 40, 50 };
    Array<uint8_t> array { storage };

    auto sub_array = array.sub_array( 1 );
    ASSERT_EQ( sub_array.size(), 4 );
    ASSERT_EQ( sub_array[0], 20 );
    ASSERT_EQ( sub_array[1], 30 );
    ASSERT_EQ( sub_array[2], 40 );
    ASSERT_EQ( sub_array[3], 50 );

    sub_array = array.sub_array( 2, 2 );
    ASSERT_EQ( sub_array.size(), 2 );
    ASSERT_EQ( sub_array[0], 30 );
    ASSERT_EQ( sub_array[1], 40 );

    sub_array = array.sub_array( 0, 3 );
    ASSERT_EQ( sub_array.size(), 3 );
    ASSERT_EQ( sub_array[0], 10 );
    ASSERT_EQ( sub_array[1], 20 );
    ASSERT_EQ( sub_array[2], 30 );

    sub_array = array.sub_array( 3, 10 );
    ASSERT_EQ( sub_array.size(), 2 );
    ASSERT_EQ( sub_array[0], 40 );
    ASSERT_EQ( sub_array[1], 50 );

    sub_array = array.sub_array( 5 );
    ASSERT_EQ( sub_array.size(), 0 );
}

TEST( array, equality_operator )
{
    uint8_t storage1[] = { 1, 2, 3, 4, 5 };
    uint8_t storage2[] = { 1, 2, 3, 4, 5 };
    uint8_t storage3[] = { 1, 2, 3, 4, 6 };
    uint8_t storage4[] = { 1, 2, 3, 4 };

    Array<uint8_t> array1 { storage1 };
    Array<uint8_t> array2 { storage2 };
    Array<uint8_t> array3 { storage3 };
    Array<uint8_t> array4 { storage4 };

    ASSERT_TRUE( array1 == array2 );

    ASSERT_FALSE( array1 == array3 );

    ASSERT_FALSE( array1 == array4 );
}

TEST( array, equality_compares_values_instead_of_representations )
{
    float positive[] = { 0.0f };
    float negative[] = { -0.0f };

    EXPECT_TRUE( Array<float>( positive ) == Array<float>( negative ) );
}

TEST( array, mutable_array_converts_to_const_element_array )
{
    Array<uint8_t, 3> mutable_array { 1, 2, 3 };
    Array<const uint8_t> const_array = mutable_array;

    static_assert( std::is_same_v<decltype( const_array.storage() ), const uint8_t*> );

    ASSERT_EQ( const_array.size(), mutable_array.size() );
    ASSERT_EQ( const_array[0], 1 );
    ASSERT_EQ( const_array[1], 2 );
    ASSERT_EQ( const_array[2], 3 );

    ASSERT_TRUE( mutable_array == const_array );
    ASSERT_TRUE( const_array == mutable_array );
}

TEST( array, initializer_list_assignment )
{
    Array<uint8_t, 5> numbers;

    numbers = { 1, 2, 3, 4, 5 };

    ASSERT_EQ( numbers[0], 1 );
    ASSERT_EQ( numbers[1], 2 );
    ASSERT_EQ( numbers[2], 3 );
    ASSERT_EQ( numbers[3], 4 );
    ASSERT_EQ( numbers[4], 5 );

    numbers = { 6, 7, 8, 9, 10 };

    ASSERT_EQ( numbers[0], 6 );
    ASSERT_EQ( numbers[1], 7 );
    ASSERT_EQ( numbers[2], 8 );
    ASSERT_EQ( numbers[3], 9 );
    ASSERT_EQ( numbers[4], 10 );
}


TEST( array, initializer_list_ctor )
{
    Array<uint8_t, 5> array1 { 1, 2, 3, 4, 5 };

    ASSERT_EQ( array1[0], 1 );
    ASSERT_EQ( array1[1], 2 );
    ASSERT_EQ( array1[2], 3 );
    ASSERT_EQ( array1[3], 4 );
    ASSERT_EQ( array1[4], 5 );

    Array<uint8_t, 5> array2 { 6, 7, 8, 9, 10 };

    ASSERT_EQ( array2[0], 6 );
    ASSERT_EQ( array2[1], 7 );
    ASSERT_EQ( array2[2], 8 );
    ASSERT_EQ( array2[3], 9 );
    ASSERT_EQ( array2[4], 10 );

    array2 = array1;

    ASSERT_EQ( array2[0], 1 );
    ASSERT_EQ( array2[1], 2 );
    ASSERT_EQ( array2[2], 3 );
    ASSERT_EQ( array2[3], 4 );
    ASSERT_EQ( array2[4], 5 );
}
