// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include "soma/containers/linked_list.h"

using namespace SOMA;

class Data
{
public:
    Data( int data )
        : m_data( data )
    {
        // intentionally empty
    }

    int m_data;

private:
    STATIC_LINKED_LIST_ITEM( Data );
};

TEST( linked_list, insert )
{
    Data data1( 1 );
    Data data2( 2 );
    Data data3( 3 );
    Linked_list<Data> list;

    ASSERT_EQ( list.size(), 0 );

    list.insert( data1 );

    ASSERT_EQ( list.size(), 1 );

    list.insert( data2 );

    ASSERT_EQ( list.size(), 2 );

    list.insert( data3 );

    ASSERT_EQ( list.size(), 3 );
}

TEST( linked_list, insert_front )
{
    Data data1( 1 );
    Data data2( 2 );
    Data data3( 3 );
    Linked_list<Data> list;

    ASSERT_EQ( list.size(), 0 );

    list.insert_front( data1 );

    ASSERT_EQ( list.size(), 1 );

    list.insert_front( data2 );

    ASSERT_EQ( list.size(), 2 );

    list.insert_front( data3 );

    ASSERT_EQ( list.size(), 3 );
}

TEST( linked_list, traversing )
{
    Data data1( 1 );
    Data data2( 2 );
    Data data3( 3 );
    Linked_list<Data> list;

    list.insert( data1 );
    list.insert( data2 );
    list.insert( data3 );

    int i = 1;

    for( Data& data: list )
    {
        ASSERT_EQ( data.m_data, i );
        ++i;
    }
}

TEST( linked_list, traversing_front )
{
    Data data1( 1 );
    Data data2( 2 );
    Data data3( 3 );
    Linked_list<Data> list;

    list.insert_front( data1 );
    list.insert_front( data2 );
    list.insert_front( data3 );

    int i = 3;

    for( Data& data: list )
    {
        ASSERT_EQ( data.m_data, i );
        --i;
    }
}

TEST( linked_list, clear )
{
    Data data1( 1 );
    Data data2( 2 );
    Data data3( 3 );
    Linked_list<Data> list;

    list.insert_front( data1 );
    list.insert_front( data2 );
    list.insert_front( data3 );

    ASSERT_EQ( list.size(), 3 );

    list.clear();

    ASSERT_EQ( list.size(), 0 );
}
