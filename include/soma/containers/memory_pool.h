// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_MEMORY_POOL_H_
#define SOMA_MEMORY_POOL_H_

#include "soma/array.h"
#include "soma/assert.h"

#include <cstddef>
#include <cstring>

namespace SOMA
{
    template<typename T, size_t N = dynamic_extent>
    class Memory_pool;

    ///
    /// @brief Memory pool for fixed-size block allocation using free list
    ///
    /// Provides O(1) allocation and deallocation of fixed-size memory blocks
    /// from a pre-allocated memory region using an intrusive free list.
    /// Thread-safety must be ensured by caller.
    ///
    /// @tparam T Underlying type of memory blocks
    ///
    /// @note Each block must occupy at least sizeof(void*) bytes to store a free-list pointer.
    ///
    /// Memory_pool<T> uses external storage. Memory_pool<T, N> owns N elements
    /// of storage and derives from Memory_pool<T>; block_size is measured in
    /// elements of T, so capacity is N / block_size blocks.
    ///
    /// @code
    /// #include <cstdint>
    /// using SOMA::Array;
    /// using SOMA::Memory_pool;
    /// void memory_pool_examples()
    /// {
    ///     uint8_t external[4 * sizeof(void*)]{};
    ///     Memory_pool<uint8_t> view { Array<uint8_t>{ external }, sizeof(void*) };
    ///     Memory_pool<uint8_t, 4 * sizeof(void*)> owned { sizeof(void*) };
    ///     Memory_pool<uint8_t>& common = owned;
    ///
    ///     auto* block = common.allocate();
    ///     common.deallocate( block );
    ///     (void)view;
    /// }
    /// @endcode
    template<typename T>
    class Memory_pool<T, dynamic_extent>
    {
    public:
        constexpr Memory_pool( Array<T> storage,
                               size_t block_size );
        Memory_pool( const Memory_pool& ) = delete;
        Memory_pool( Memory_pool&& ) = delete;
        Memory_pool& operator=( const Memory_pool& ) = delete;
        Memory_pool& operator=( Memory_pool&& ) = delete;

        T* allocate( void );
        void deallocate( T* p_block );

        constexpr size_t capacity( void ) const;

        size_t size( void ) const;
        size_t space( void ) const;

        constexpr size_t block_size( void ) const;

    private:
        Array<T> m_storage;
        const size_t mc_block_size;
        const size_t mc_block_count;
        size_t m_free_count;
        void* mp_free_list_head = nullptr; // Head of free list

        bool is_from_pool( const T* p_block ) const;
    };

    template<typename T>
    constexpr Memory_pool<T, dynamic_extent>::Memory_pool( Array<T> storage,
                                                           size_t block_size )
        : m_storage( storage ),
          mc_block_size( block_size ),
          mc_block_count( block_size == 0u ? 0u : storage.size() / block_size ),
          m_free_count( mc_block_count )
    {
        ASSERT( block_size > 0u );
        ASSERT( storage.size() % block_size == 0 );             // Storage must be multiple of block size
        ASSERT( mc_block_count > 0 );                           // Must have at least one block
        ASSERT( block_size * sizeof( T ) >= sizeof( void* ) );  // Block must fit a pointer for free list

        // Store links as bytes so blocks need no alignment beyond alignof(T).
        void* p_current = m_storage.storage();

        mp_free_list_head = p_current;

        for( size_t i = 0; i < mc_block_count - 1; i++ )
        {
            void* p_next = &m_storage[( i + 1 ) * mc_block_size];

            std::memcpy( p_current, &p_next, sizeof( p_next ) );
            p_current = p_next;
        }

        // Last block points to nullptr
        void* p_end = nullptr;

        std::memcpy( p_current, &p_end, sizeof( p_end ) );
    }

    template<typename T>
    T* Memory_pool<T, dynamic_extent>::allocate( void )
    {
        T* rv_p_block = nullptr;

        // Pop from free list - O(1)
        if( mp_free_list_head != nullptr )
        {
            rv_p_block = static_cast<T*>( mp_free_list_head );

            void* p_next = nullptr;

            std::memcpy( &p_next, mp_free_list_head, sizeof( p_next ) );
            mp_free_list_head = p_next; // Advance head to next free block

            m_free_count--;
        }

        return rv_p_block;
    }

    template<typename T>
    void Memory_pool<T, dynamic_extent>::deallocate( T* p_block )
    {
        ASSERT( p_block != nullptr );
        ASSERT( is_from_pool( p_block ) );

        // Push to front of free list - O(1)
        std::memcpy( p_block, &mp_free_list_head, sizeof( mp_free_list_head ) );
        mp_free_list_head = p_block;

        m_free_count++;
    }

    template<typename T>
    constexpr size_t Memory_pool<T, dynamic_extent>::capacity( void ) const
    {
        return mc_block_count;
    }

    template<typename T>
    size_t Memory_pool<T, dynamic_extent>::space( void ) const
    {
        return m_free_count;
    }

    template<typename T>
    size_t Memory_pool<T, dynamic_extent>::size( void ) const
    {
        return mc_block_count - m_free_count;
    }

    template<typename T>
    constexpr size_t Memory_pool<T, dynamic_extent>::block_size( void ) const
    {
        return mc_block_size;
    }

    template<typename T>
    bool Memory_pool<T, dynamic_extent>::is_from_pool( const T* p_block ) const
    {
        const T* p_start = m_storage.storage();
        const T* p_end = p_start + m_storage.size();

        return ( p_block >= p_start ) && ( p_block < p_end );
    }

    namespace Internal
    {
        template<typename T, size_t N>
        struct Memory_pool_storage_t
        {
            Memory_pool_storage_t()
            {
            }

            T m_storage[N];
        };
    }

    ///
    /// @brief Memory pool with N elements of owned storage.
    ///
    /// The storage base is constructed before the pool initializes its free
    /// list. N counts elements of T; capacity is N / block_size blocks.
    /// Owning pools cannot be copied or moved because free-list links point
    /// into their own storage.
    template<typename T, size_t N>
    class Memory_pool: private Internal::Memory_pool_storage_t<T, N>,
        public Memory_pool<T, dynamic_extent>
    {
    public:
        static_assert( N > 0u, "Memory pool storage must not be empty" );

        explicit Memory_pool( size_t block_size );
        Memory_pool( const Memory_pool& ) = delete;
        Memory_pool( Memory_pool&& ) = delete;
        Memory_pool& operator=( const Memory_pool& ) = delete;
        Memory_pool& operator=( Memory_pool&& ) = delete;

    private:
        using Storage = Internal::Memory_pool_storage_t<T, N>;
    };

    template<typename T, size_t N>
    Memory_pool<T, N>::Memory_pool( size_t block_size )
        : Storage(),
          Memory_pool<T, dynamic_extent>( Array<T>( Storage::m_storage, N ), block_size )
    {
    }
} // namespace SOMA

#endif // MEMORY_POOL_H_
