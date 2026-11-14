// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"

#include <type_traits>

#include "soma/containers/memory_pool.h"

using namespace SOMA;

TEST( memory_pool, owned_storage )
{
    using Owned_pool = Memory_pool<uint8_t, 40>;
    static_assert( std::is_base_of_v<Memory_pool<uint8_t>, Owned_pool> );
    static_assert( !std::is_copy_constructible_v<Memory_pool<uint8_t>> );
    static_assert( !std::is_copy_constructible_v<Owned_pool> );
    static_assert( !std::is_move_constructible_v<Owned_pool> );

    Owned_pool owned( 8 );
    Memory_pool<uint8_t>& pool = owned;
    ASSERT_EQ( pool.capacity(), 5 );
    ASSERT_EQ( pool.block_size(), 8 );

    uint8_t* blocks[5]{};
    for( size_t i = 0; i < 5; ++i )
    {
        blocks[i] = pool.allocate();
        ASSERT_NE( blocks[i], nullptr );
        blocks[i][0] = static_cast<uint8_t>( i + 1 );
    }
    EXPECT_EQ( pool.allocate(), nullptr );
    EXPECT_EQ( pool.space(), 0 );

    Owned_pool other( 8 );
    uint8_t* other_block = other.allocate();
    ASSERT_NE( other_block, nullptr );
    EXPECT_NE( other_block, blocks[0] );
    other.deallocate( other_block );

    pool.deallocate( blocks[2] );
    EXPECT_EQ( pool.allocate(), blocks[2] );
    EXPECT_EQ( pool.size(), 5 );

    for( uint8_t* block: blocks )
    {
        pool.deallocate( block );
    }
    EXPECT_EQ( pool.space(), 5 );
}

TEST( memory_pool, owned_storage_with_unaligned_block_stride )
{
    Memory_pool<uint8_t, 30> pool( 10 );
    uint8_t* first = pool.allocate();
    uint8_t* second = pool.allocate();
    uint8_t* third = pool.allocate();

    ASSERT_NE( first, nullptr );
    ASSERT_NE( second, nullptr );
    ASSERT_NE( third, nullptr );
    EXPECT_EQ( second - first, 10 );
    EXPECT_EQ( third - second, 10 );

    pool.deallocate( second );
    EXPECT_EQ( pool.allocate(), second );
    pool.deallocate( first );
    pool.deallocate( second );
    pool.deallocate( third );
    EXPECT_EQ( pool.space(), 3 );
}

TEST( memory_pool, owned_storage_extent_counts_elements )
{
    Memory_pool<uint32_t, 10> pool( 2 );
    EXPECT_EQ( pool.capacity(), 5 );

    uint32_t* block = pool.allocate();
    ASSERT_NE( block, nullptr );
    block[0] = 42;
    EXPECT_EQ( block[0], 42 );
    pool.deallocate( block );
    EXPECT_EQ( pool.space(), 5 );
}

TEST( memory_pool, external_storage_need_not_be_pointer_aligned )
{
    alignas(void*) uint8_t storage[31];
    Memory_pool<uint8_t> pool( Array<uint8_t>( storage + 1, 30 ), 10 );

    uint8_t* first = pool.allocate();
    uint8_t* second = pool.allocate();
    ASSERT_EQ( first, storage + 1 );
    ASSERT_EQ( second, storage + 11 );

    pool.deallocate( first );
    EXPECT_EQ( pool.allocate(), first );
}

// Test with simple types
TEST( memory_pool, basic_allocation_byte )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    ASSERT_EQ( pool.capacity(), 10 );
    ASSERT_EQ( pool.space(), 10 );
    ASSERT_EQ( pool.size(), 0 );
    ASSERT_EQ( pool.block_size(), 10 );

    uint8_t* p_block = pool.allocate();
    ASSERT_NE( p_block, nullptr );
    ASSERT_EQ( pool.space(), 9 );
    ASSERT_EQ( pool.size(), 1 );

    pool.deallocate( p_block );
    ASSERT_EQ( pool.space(), 10 );
    ASSERT_EQ( pool.size(), 0 );
}

TEST( memory_pool, basic_allocation_uint32 )
{
    uint32_t storage[100];
    Array<uint32_t> array( storage, 100 );
    Memory_pool<uint32_t> pool( array, 5 );  // 5 uint32_t per block

    ASSERT_EQ( pool.capacity(), 20 );
    ASSERT_EQ( pool.space(), 20 );
    ASSERT_EQ( pool.block_size(), 5 );

    uint32_t* p_block = pool.allocate();
    ASSERT_NE( p_block, nullptr );
    ASSERT_EQ( pool.space(), 19 );
    ASSERT_EQ( pool.size(), 1 );
}

TEST( memory_pool, allocate_all_blocks )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    uint8_t* blocks[10];

    // Allocate all blocks
    for( size_t i = 0; i < 10; i++ )
    {
        blocks[i] = pool.allocate();
        ASSERT_NE( blocks[i], nullptr );
        ASSERT_EQ( pool.size(), i + 1 );
        ASSERT_EQ( pool.space(), 10 - i - 1 );
    }

    // Pool should be exhausted
    uint8_t* p_null = pool.allocate();
    ASSERT_EQ( p_null, nullptr );
    ASSERT_EQ( pool.space(), 0 );
    ASSERT_EQ( pool.size(), 10 );

    // Deallocate all blocks
    for( size_t i = 0; i < 10; i++ )
    {
        pool.deallocate( blocks[i] );
        ASSERT_EQ( pool.space(), i + 1 );
        ASSERT_EQ( pool.size(), 10 - i - 1 );
    }

    ASSERT_EQ( pool.space(), 10 );
    ASSERT_EQ( pool.size(), 0 );
}

TEST( memory_pool, deallocate_reuse )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    // Allocate 3 blocks
    uint8_t* p_block1 = pool.allocate();
    uint8_t* p_block2 = pool.allocate();
    uint8_t* p_block3 = pool.allocate();

    ASSERT_NE( p_block1, nullptr );
    ASSERT_NE( p_block2, nullptr );
    ASSERT_NE( p_block3, nullptr );
    ASSERT_EQ( pool.size(), 3 );

    // Deallocate middle block
    pool.deallocate( p_block2 );
    ASSERT_EQ( pool.size(), 2 );

    // Allocate again - should reuse deallocated block
    uint8_t* p_block4 = pool.allocate();
    ASSERT_NE( p_block4, nullptr );
    ASSERT_EQ( pool.size(), 3 );

    pool.deallocate( p_block1 );
    pool.deallocate( p_block3 );
    pool.deallocate( p_block4 );
}

TEST( memory_pool, multiple_alloc_dealloc_cycles )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    // Cycle 1
    uint8_t* p1 = pool.allocate();
    uint8_t* p2 = pool.allocate();
    ASSERT_EQ( pool.size(), 2 );
    pool.deallocate( p1 );
    pool.deallocate( p2 );
    ASSERT_EQ( pool.size(), 0 );

    // Cycle 2
    uint8_t* p3 = pool.allocate();
    uint8_t* p4 = pool.allocate();
    uint8_t* p5 = pool.allocate();
    ASSERT_EQ( pool.size(), 3 );
    pool.deallocate( p3 );
    pool.deallocate( p4 );
    pool.deallocate( p5 );
    ASSERT_EQ( pool.size(), 0 );

    // Pool should still be fully available
    ASSERT_EQ( pool.space(), 10 );
}

TEST( memory_pool, interleaved_alloc_dealloc )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    uint8_t* p1 = pool.allocate();
    uint8_t* p2 = pool.allocate();
    ASSERT_EQ( pool.size(), 2 );

    pool.deallocate( p1 );
    ASSERT_EQ( pool.size(), 1 );

    uint8_t* p3 = pool.allocate();
    ASSERT_EQ( pool.size(), 2 );

    pool.deallocate( p2 );
    ASSERT_EQ( pool.size(), 1 );

    uint8_t* p4 = pool.allocate();
    ASSERT_EQ( pool.size(), 2 );

    pool.deallocate( p3 );
    pool.deallocate( p4 );
    ASSERT_EQ( pool.size(), 0 );
}

TEST( memory_pool, block_uniqueness )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    uint8_t* blocks[10];

    // Allocate all blocks and verify they're all unique
    for( size_t i = 0; i < 10; i++ )
    {
        blocks[i] = pool.allocate();
        ASSERT_NE( blocks[i], nullptr );

        // Check that this block is unique
        for( size_t j = 0; j < i; j++ )
        {
            ASSERT_NE( blocks[i], blocks[j] );
        }
    }

    // Clean up
    for( size_t i = 0; i < 10; i++ )
    {
        pool.deallocate( blocks[i] );
    }
}

TEST( memory_pool, write_to_allocated_blocks )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    Memory_pool<uint8_t> pool( array, 10 );

    uint8_t* p_block1 = pool.allocate();
    uint8_t* p_block2 = pool.allocate();

    // Write data to blocks
    for( size_t i = 0; i < 10; i++ )
    {
        p_block1[i] = static_cast<uint8_t>( i );
        p_block2[i] = static_cast<uint8_t>( 100 + i );
    }

    // Verify data
    for( size_t i = 0; i < 10; i++ )
    {
        ASSERT_EQ( p_block1[i], i );
        ASSERT_EQ( p_block2[i], 100 + i );
    }

    pool.deallocate( p_block1 );
    pool.deallocate( p_block2 );
}

// Test with struct
struct Test_struct
{
    uint32_t value1;
    uint32_t value2;
    uint8_t data[8];
};

TEST( memory_pool, struct_allocation )
{
    Test_struct storage[10];
    Array<Test_struct> array( storage, 10 );
    Memory_pool<Test_struct> pool( array, 1 );  // 1 struct per block

    ASSERT_EQ( pool.capacity(), 10 );
    ASSERT_EQ( pool.block_size(), 1 );

    Test_struct* p_block = pool.allocate();
    ASSERT_NE( p_block, nullptr );

    // Use the allocated struct
    p_block->value1 = 0x12345678;
    p_block->value2 = 0xABCDEF00;
    p_block->data[0] = 0xAA;

    ASSERT_EQ( p_block->value1, 0x12345678 );
    ASSERT_EQ( p_block->value2, 0xABCDEF00 );
    ASSERT_EQ( p_block->data[0], 0xAA );

    pool.deallocate( p_block );
}

TEST( memory_pool, minimum_block_size )
{
    // Block size must be at least sizeof(void*) to store free list pointers
    // Using 64 bytes which is divisible by both 8 and any pointer size
    uint8_t storage[64];
    Array<uint8_t> array( storage, 64 );

    // 8 blocks of 8 bytes each
    Memory_pool<uint8_t> pool( array, 8 );
    ASSERT_EQ( pool.capacity(), 8 );
}

TEST( memory_pool, large_blocks )
{
    uint8_t storage[1000];
    Array<uint8_t> array( storage, 1000 );
    Memory_pool<uint8_t> pool( array, 100 );  // Large blocks

    ASSERT_EQ( pool.capacity(), 10 );
    ASSERT_EQ( pool.block_size(), 100 );

    uint8_t* p_block = pool.allocate();
    ASSERT_NE( p_block, nullptr );

    // Write to entire block
    for( size_t i = 0; i < 100; i++ )
    {
        p_block[i] = static_cast<uint8_t>( i % 256 );
    }

    // Verify
    for( size_t i = 0; i < 100; i++ )
    {
        ASSERT_EQ( p_block[i], i % 256 );
    }

    pool.deallocate( p_block );
}

TEST( memory_pool, single_block )
{
    uint8_t storage[10];
    Array<uint8_t> array( storage, 10 );
    Memory_pool<uint8_t> pool( array, 10 );  // Only 1 block

    ASSERT_EQ( pool.capacity(), 1 );
    ASSERT_EQ( pool.space(), 1 );

    uint8_t* p_block = pool.allocate();
    ASSERT_NE( p_block, nullptr );
    ASSERT_EQ( pool.space(), 0 );

    uint8_t* p_null = pool.allocate();
    ASSERT_EQ( p_null, nullptr );

    pool.deallocate( p_block );
    ASSERT_EQ( pool.space(), 1 );
}

TEST( memory_pool, stress_test )
{
    uint8_t storage[1000];
    Array<uint8_t> array( storage, 1000 );
    Memory_pool<uint8_t> pool( array, 10 );

    uint8_t* blocks[100] = { nullptr };

    // Allocate and deallocate in random patterns
    for( size_t iteration = 0; iteration < 5; iteration++ )
    {
        // Allocate all blocks
        for( size_t i = 0; i < 100; i++ )
        {
            blocks[i] = pool.allocate();
            ASSERT_NE( blocks[i], nullptr );
        }
        ASSERT_EQ( pool.size(), 100 );
        ASSERT_EQ( pool.allocate(), nullptr );  // Pool exhausted

        // Deallocate odd indices
        for( size_t i = 1; i < 100; i += 2 )
        {
            pool.deallocate( blocks[i] );
            blocks[i] = nullptr;
        }
        ASSERT_EQ( pool.size(), 50 );

        // Reallocate the freed blocks
        for( size_t i = 1; i < 100; i += 2 )
        {
            blocks[i] = pool.allocate();
            ASSERT_NE( blocks[i], nullptr );
        }
        ASSERT_EQ( pool.size(), 100 );

        // Deallocate all
        for( size_t i = 0; i < 100; i++ )
        {
            pool.deallocate( blocks[i] );
            blocks[i] = nullptr;
        }
        ASSERT_EQ( pool.size(), 0 );
        ASSERT_EQ( pool.space(), 100 );
    }
}

TEST( memory_pool, constexpr_methods )
{
    uint8_t storage[100];
    Array<uint8_t> array( storage, 100 );
    constexpr size_t block_size = 10;
    Memory_pool<uint8_t> pool( array, block_size );

    // These methods are constexpr so they can be called on const objects
    size_t total = pool.capacity();
    size_t bsize = pool.block_size();

    ASSERT_EQ( total, 10 );
    ASSERT_EQ( bsize, 10 );
}
