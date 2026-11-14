// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_RING_BUFFER_H_
#define SOMA_RING_BUFFER_H_

#include <atomic>
#include <algorithm>

#include "soma/assert.h"
#include "soma/containers/buffer.h"

namespace SOMA
{
    ///
    /// @brief Class representing a FIFO queue implemented using a ring buffer.
    ///        - thread safe for single-producer/single-consumer usage (i.e. consumers and producers don't need to sync to each other)
    ///        - if there is more consumers, they must sync to each other
    ///        - if there is more producers, they must sync to each other
    ///        - API for consumer to access the buffer directly to avoid copying (up to amount of continuously available items
    ///        - data overwriting is not supported
    ///
    /// Ring_buffer<T> uses external storage. Ring_buffer<T, N> owns N slots
    /// and derives from Ring_buffer<T>. Capacity must be positive, and external
    /// storage must outlive the ring buffer.
    ///
    /// @code
    /// #include <cstdint>
    /// using SOMA::Array;
    /// using SOMA::Ring_buffer;
    /// void ring_buffer_examples()
    /// {
    ///     uint8_t external[4]{};
    ///     Ring_buffer<uint8_t> view { Array<uint8_t>{ external } };
    ///     Ring_buffer<uint8_t, 4> owned;
    ///     Ring_buffer<uint8_t>& common = owned;
    ///
    ///     view.insert( 1 );
    ///     owned.insert( 2 );
    ///     uint8_t item{};
    ///     common.remove( item );
    /// }
    /// @endcode
    ///
    /// @param T     type of the data item
    template<typename T, size_t N = dynamic_extent>
    class Ring_buffer;

    template<typename T>
    class Ring_buffer<T, dynamic_extent>
    {
    public:
        static_assert( std::atomic_size_t::is_always_lock_free,
                       "Ring_buffer requires lock-free size_t atomics" );

        explicit Ring_buffer( Array<T> array );

        size_t capacity( void ) const;

        void clear( void );

        size_t size( void ) const;
        size_t space( void ) const;
        bool is_empty( void ) const;
        bool is_full( void ) const;

        bool insert( const T& item );
        size_t insert( Buffer_view<T> items );

        bool remove( T& out_item );
        size_t remove( Buffer<T>& out_buffer );
        size_t remove( Buffer<T>& out_buffer,
                       size_t max_count );

        Buffer<T> get_space( void ) const;
        void place( size_t count );

        Buffer_view<T> view_items( void ) const;
        void displace( size_t count );

    private:
        Array<T> m_array;

        // Releases publish completed writes or reads; acquires make them visible
        // to the other side before it accesses or reuses the corresponding slots.
        std::atomic_size_t m_items_cnt = 0u;

        size_t m_first_full_idx = 0u; // index of the first occupied position (the oldest item)
        size_t m_first_free_idx = 0u; // index of the first free position
    };

    ///
    /// @brief Ctor.
    /// @param p_buffer pointer to the ring buffer's storage
    /// @param size     size of the provided buffer
    template<typename T>
    Ring_buffer<T, dynamic_extent>::Ring_buffer( Array<T> array )
        : m_array( array )
    {
        ASSERT( array.size() > 0u );
    }

    ///
    /// @brief Clears the ring buffer.
    template<typename T>
    void Ring_buffer<T, dynamic_extent>::clear( void )
    {
        displace( size() );
    }

    ///
    /// @brief Retrieves ring buffer capacity.
    /// @retval ring buffer capacity
    template<typename T>
    size_t Ring_buffer<T, dynamic_extent>::capacity( void ) const
    {
        return m_array.size();
    }

    ///
    /// @brief Inserts an item to the buffer.
    /// @param item to insert
    /// @retval true, if the item was successfully inserted
    template<typename T>
    bool Ring_buffer<T, dynamic_extent>::insert( const T& item )
    {
        return insert( Buffer_view( Array<const T>( &item, 1u ) ) ) == 1u;
    }

    ///
    /// @brief Inserts an array of items to the buffer.
    /// @param p_items pointer to the source buffer
    /// @param count   number of items in the source buffer
    /// @retval number of items inserted (if less than 'count', the rest did not fit)
    template<typename T>
    size_t Ring_buffer<T, dynamic_extent>::insert( Buffer_view<T> items )
    {
        size_t rv_copied = capacity() - m_items_cnt.load( std::memory_order_acquire ); // init with current number of free positions

        rv_copied = std::min( rv_copied, items.size() );

        for( size_t i = 0; i < rv_copied; ++i )
        {
            m_array[ m_first_free_idx ] = items[i];

            ++m_first_free_idx;

            if( m_first_free_idx >= capacity() )
            // end of the buffer reached, wrap around
            {
                m_first_free_idx = 0;
            }
        }

        // atomically loads, adds and stores the variable
        // NOTE: 'std::memory_order_release' ensures the data in 'm_array' is visible before this operation, i.e.
        //       are not re-ordered neither by the compiler nor the CPU
        m_items_cnt.fetch_add( rv_copied, std::memory_order_release );

        return rv_copied;
    }

    ///
    /// @brief Formally inserts externally copied items to the ring buffer.
    /// @param count   number of items copied to the buffer externally
    template<typename T>
    void Ring_buffer<T, dynamic_extent>::place( size_t count )
    {
        ASSERT( count <= space() );

        size_t free_to_end = capacity() - m_first_free_idx;

        if( count < free_to_end )
        {
            m_first_free_idx += count;
        }
        else
        // wrap around
        {
            m_first_free_idx = count - free_to_end;
        }

        // atomically loads, adds and stores the variable
        // NOTE: 'std::memory_order_release' ensures the data in 'm_array' is visible before this operation, i.e.
        //       are not re-ordered neither by the compiler nor the CPU
        m_items_cnt.fetch_add( count, std::memory_order_release );
    }

    ///
    /// @brief Removes one item (if available) from the buffer and copies it to the provided variable.
    /// @param out_item whete the removed item is copied
    /// @retval true, if an item was successfully removed
    template<typename T>
    bool Ring_buffer<T, dynamic_extent>::remove( T& out_item )
    {
        bool rv_removed = false;

        if( m_items_cnt.load( std::memory_order_acquire ) > 0u )
        {
            out_item = m_array[ m_first_full_idx ];

            ++m_first_full_idx;
            if( m_first_full_idx >= capacity() )
            // wrap around
            {
                m_first_full_idx = 0;
            }

            // atomically loads, subtracks and stores the variable
            // NOTE: 'std::memory_order_release' ensures the 'mp_buffer' is copied out before this operation, i.e.
            //       are not re-ordered neither by the compiler nor the CPU
            m_items_cnt.fetch_sub( 1, std::memory_order_release );

            rv_removed = true;
        }

        return rv_removed;
    }

    ///
    /// @brief Removes items (if available) to the provided buffer.
    /// @param out_buffer destination buffer
    /// @retval number of items removed
    template<typename T>
    size_t Ring_buffer<T, dynamic_extent>::remove( Buffer<T>& out_buffer )
    {
        return remove( out_buffer, out_buffer.space() );
    }

    ///
    /// @brief Removes specified amount of items (if available) to the provided buffer.
    /// @param out_buffer destination buffer
    /// @param max_count  maximum number of items to remove
    /// @retval number of items removed (if less than 'max_count', there was not enough items)
    template<typename T>
    size_t Ring_buffer<T, dynamic_extent>::remove( Buffer<T>& out_buffer,
                                                   size_t max_count )
    {
        size_t rv_copied = 0u;
        size_t remaining_count = std::min( max_count, out_buffer.space() );

        while( remaining_count > 0u )
        {
            Buffer_view items = view_items();

            if( items.size() == 0u )
            {
                break;
            }

            items = items.sub_buffer( 0u, std::min( items.size(), remaining_count ) );

            size_t inserted = out_buffer.insert( items );

            if( inserted == 0u )
            {
                break;
            }

            displace( inserted );
            rv_copied += inserted;
            remaining_count -= inserted;
        }

        return rv_copied;
    }

    ///
    /// @brief Deletes specified amount of items (if available).
    /// @param count number of items to delete
    /// @retval number of items deleted (if less than 'count', there was not enough items)
    template<typename T>
    void Ring_buffer<T, dynamic_extent>::displace( size_t count )
    {
        ASSERT( count <= size() );

        size_t cnt_to_end = capacity() - m_first_full_idx;

        if( count < cnt_to_end )
        {
            m_first_full_idx += count;
        }
        else
        // wrap around
        {
            m_first_full_idx = count - cnt_to_end;
        }

        // atomically loads, subtracks and stores the variable
        // NOTE: 'std::memory_order_release' ensures the 'm_array' is copied out before this operation, i.e.
        //       are not re-ordered neither by the compiler nor the CPU
        m_items_cnt.fetch_sub( count, std::memory_order_release );
    }

    ///
    /// @brief Retrieves whether the buffer is empty.
    /// @retval true, if the buffer is empty
    ///         false otherwise
    template<typename T>
    bool Ring_buffer<T, dynamic_extent>::is_empty( void ) const
    {
        return ( size() == 0 );
    }

    ///
    /// @brief Retrieves whether the buffer is full.
    /// @retval true, if the buffer is full
    ///         false otherwise
    template<typename T>
    bool Ring_buffer<T, dynamic_extent>::is_full( void ) const
    {
        return ( space() == 0 );
    }

    ///
    /// @brief Retrieves number of items in the buffer.
    /// @retval number of items in the buffer
    template<typename T>
    size_t Ring_buffer<T, dynamic_extent>::size( void ) const
    {
        return m_items_cnt.load( std::memory_order_acquire );
    }

    ///
    /// @brief Retrieves number of free positions in the buffer.
    /// @retval number of free positions in the buffer
    template<typename T>
    size_t Ring_buffer<T, dynamic_extent>::space( void ) const
    {
        return capacity() - m_items_cnt.load( std::memory_order_acquire );
    }

    ///
    /// @brief Retrieves buffered items.
    ///        NOTE: Retrieved buffer does not necessarily contains all the items as it must be continuous.
    /// @retval view of the items in the buffer
    template<typename T>
    Buffer_view<T> Ring_buffer<T, dynamic_extent>::view_items( void ) const
    {
        size_t continuous_item_cnt = m_items_cnt.load( std::memory_order_acquire );
        size_t positions_to_end = capacity() - m_first_full_idx;

        continuous_item_cnt = std::min( continuous_item_cnt, positions_to_end );

        return Buffer_view( m_array.sub_array( m_first_full_idx, continuous_item_cnt ) );
    }

    template<typename T>
    Buffer<T> Ring_buffer<T, dynamic_extent>::get_space( void ) const
    {
        Buffer<T> rv_free_buffer;
        size_t continuous_free_cnt = capacity() - m_items_cnt.load( std::memory_order_acquire );
        size_t positions_to_end = capacity() - m_first_free_idx;

        continuous_free_cnt = std::min( continuous_free_cnt, positions_to_end );

        if( continuous_free_cnt > 0u )
        {
            rv_free_buffer = Buffer<T>( m_array.sub_array( m_first_free_idx, continuous_free_cnt ) );
        }

        return rv_free_buffer;
    }

    template<typename T, size_t N>
    class Ring_buffer: public Ring_buffer<T, dynamic_extent>
    {
    public:
        static_assert( N > 0u, "Ring buffer capacity must be positive" );

        Ring_buffer( void );

    private:
        T m_storage[N];
    };

    template<typename T, size_t N>
    Ring_buffer<T, N>::Ring_buffer( void )
        : Ring_buffer<T, dynamic_extent>( Array( m_storage, N ) )
    {
        // intentionally empty
    }
} // namespace SOMA

#endif // RING_BUFFER_H_
