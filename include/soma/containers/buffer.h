// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BUFFER_H_
#define SOMA_BUFFER_H_

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <utility>

#include "soma/containers/buffer_view.h"
#include "soma/assert.h"
#include "soma/array.h"

namespace SOMA
{
    ///
    /// @brief A buffer over external storage or a buffer with owned storage.
    ///
    /// Buffer<T> uses storage supplied by the caller. Buffer<T, N> owns N
    /// elements and derives from Buffer<T>. Both start with zero items;
    /// capacity and item count are separate. External storage must outlive
    /// every buffer that refers to it.
    ///
    /// @code
    /// #include <cstdint>
    /// using SOMA::Buffer;
    /// void buffer_examples()
    /// {
    ///     uint8_t external[4]{};
    ///     Buffer<uint8_t> view { external };         // Uses caller-owned storage.
    ///     Buffer<uint8_t, 4> owned;                  // Owns four slots; size is zero.
    ///     Buffer<uint8_t, 4> filled { 1, 2 };        // Two items in four slots.
    ///
    ///     view.insert( 3 );
    ///     owned.insert( 4 );
    ///     Buffer<uint8_t> owned_view = owned;       // Shares owned's storage.
    ///     Buffer<uint8_t, 4> copy { filled };       // Copies items to new storage.
    ///     auto items = copy.view_items();           // View of the populated items.
    ///     (void)owned_view; (void)items;
    /// }
    /// @endcode
    template<typename T, size_t N = dynamic_extent>
    class Buffer;

    template<typename T>
    class Buffer<T, dynamic_extent>
    {
    public:
        Buffer() = default;
        explicit Buffer( Array<T> array );

        template<size_t N>
        explicit Buffer( T ( & p_array )[N] );

        ~Buffer() = default;

        Buffer( const Buffer& buffer ) = default;

        operator Buffer_view<T>( ) const;
        operator Array<T>( ) const;
        template<typename U = T,
                 std::enable_if_t<std::is_same<U, T>::value && !std::is_const<U>::value, int> = 0>
        operator Array<const U>( ) const;

        Buffer& operator=( const Buffer& buffer ) = default;

        size_t capacity( void ) const;

        void clear( void );

        size_t size( void ) const;
        size_t space( void ) const;
        bool is_empty( void ) const;
        bool is_full( void ) const;

        template<typename U = T>
        typename std::enable_if_t<( sizeof( U ) >= sizeof( U* ) ), bool>
        insert( const T& value );
        template<typename U = T>
        typename std::enable_if_t<( sizeof( U ) < sizeof( U* ) ), bool>
        insert( const T value );
        size_t insert( Buffer_view<T> buffer );

        Array<T> get_space( void );
        void place( size_t count );

        Buffer_view<T> view_items( void ) const;
        void displace( size_t count );

    private:
        Array<T> m_array;

        size_t m_items_cnt = 0u; // number of items in the buffer
    };

    template<typename T>
    Buffer<T, dynamic_extent>::Buffer( Array<T> array )
        : m_array( array )
    {
        // intentionally empty
    }

    template<typename T>
    template<size_t N>
    Buffer<T, dynamic_extent>::Buffer( T ( & p_buffer )[N] )
        : Buffer( Array<T>( p_buffer, N ) )
    {
        // intentionally empty
    }

    template<typename T>
    size_t Buffer<T, dynamic_extent>::capacity( void ) const
    {
        return m_array.size();
    }

    template<typename T>
    size_t Buffer<T, dynamic_extent>::size( void ) const
    {
        return m_items_cnt;
    };

    template<typename T>
    size_t Buffer<T, dynamic_extent>::space( void ) const
    {
        return m_array.size() - m_items_cnt;
    }

    template<typename T>
    bool Buffer<T, dynamic_extent>::is_empty( void ) const
    {
        return ( size() == 0 );
    }

    template<typename T>
    bool Buffer<T, dynamic_extent>::is_full( void ) const
    {
        return ( space() == 0 );
    }

    template<typename T>
    void Buffer<T, dynamic_extent>::clear( void )
    {
        m_items_cnt = 0u;
    }

    template<typename T>
    template<typename U>
    typename std::enable_if_t<( sizeof( U ) >= sizeof( U* ) ), bool>
    Buffer<T, dynamic_extent>::insert( const T& value )
    {
        bool rv_succeeded = false;

        if( space() > 0u )
        {
            m_array[m_items_cnt] = value;
            ++m_items_cnt;

            rv_succeeded = true;
        }

        return rv_succeeded;
    }

    template<typename T>
    template<typename U>
    typename std::enable_if_t<( sizeof( U ) < sizeof( U* ) ), bool>
    Buffer<T, dynamic_extent>::insert( T value )
    {
        bool rv_succeeded = false;

        if( space() > 0u )
        {
            m_array[m_items_cnt] = value;
            ++m_items_cnt;

            rv_succeeded = true;
        }

        return rv_succeeded;
    }

    template<typename T>
    size_t Buffer<T, dynamic_extent>::insert( Buffer_view<T> buffer )
    {
        size_t rv_copied = std::min( buffer.size(), space() );

        if( rv_copied > 0u )
        {
            std::copy_n( buffer.array().storage(), rv_copied, m_array.storage() + m_items_cnt );
        }

        m_items_cnt += rv_copied;

        return rv_copied;
    }

    template<typename T>
    Buffer_view<T> Buffer<T, dynamic_extent>::view_items( void ) const
    {
        return Buffer_view( Array( m_array.storage(), m_items_cnt ) );
    }

    template<typename T>
    Buffer<T, dynamic_extent>::operator Buffer_view<T>( ) const
    {
        return view_items();
    }

    template<typename T>
    Buffer<T, dynamic_extent>::operator Array<T>( ) const
    {
        return m_array.sub_array( 0u, m_items_cnt );
    }

    template<typename T>
    template<typename U, std::enable_if_t<std::is_same<U, T>::value && !std::is_const<U>::value, int>>
    Buffer<T, dynamic_extent>::operator Array<const U>( ) const
    {
        return Array<const U>( m_array.storage(), m_items_cnt );
    }

    template<typename T>
    Array<T> Buffer<T, dynamic_extent>::get_space( void )
    {
        return m_array.sub_array( m_items_cnt );
    }

    template<typename T>
    void Buffer<T, dynamic_extent>::place( size_t count )
    {
        ASSERT( count <= space() );

        m_items_cnt += count;
    }

    template<typename T>
    void Buffer<T, dynamic_extent>::displace( size_t count )
    {
        ASSERT( count <= m_items_cnt );

        m_items_cnt -= count;
    }

    template<typename T, size_t N>
    class Buffer: public Buffer<T, dynamic_extent>
    {
    public:
        static_assert( N > 0u, "Buffer capacity must be positive" );

        Buffer( void );
        Buffer( std::initializer_list<T> init_list );
        Buffer( const Buffer& other );
        Buffer( Buffer&& other );
        Buffer& operator=( const Buffer& other );
        Buffer& operator=( Buffer&& other );

    private:
        using View = Buffer<T, dynamic_extent>;

        T m_storage[N];
    };

    template<typename T, size_t N>
    Buffer<T, N>::Buffer( void )
        : View( Array( m_storage, N ) )
    {
        // intentionally empty
    }

    template<typename T, size_t N>
    Buffer<T, N>::Buffer( std::initializer_list<T> init_list )
        : Buffer()
    {
        ASSERT( init_list.size() <= N );

        size_t i = 0;

        for( const auto& item: init_list )
        {
            m_storage[i++] = item;
        }

        this->place( init_list.size() );
    }

    template<typename T, size_t N>
    Buffer<T, N>::Buffer( const Buffer& other )
        : Buffer()
    {
        std::copy_n( other.m_storage, other.size(), m_storage );
        this->place( other.size() );
    }

    template<typename T, size_t N>
    Buffer<T, N>::Buffer( Buffer&& other )
        : Buffer()
    {
        std::move( other.m_storage, other.m_storage + other.size(), m_storage );
        this->place( other.size() );
    }

    template<typename T, size_t N>
    Buffer<T, N>& Buffer<T, N>::operator=( const Buffer& other )
    {
        if( this != &other )
        {
            std::copy_n( other.m_storage, other.size(), m_storage );
            View::operator=( View( Array( m_storage, N ) ) );
            this->place( other.size() );
        }

        return *this;
    }

    template<typename T, size_t N>
    Buffer<T, N>& Buffer<T, N>::operator=( Buffer&& other )
    {
        if( this != &other )
        {
            std::move( other.m_storage, other.m_storage + other.size(), m_storage );
            View::operator=( View( Array( m_storage, N ) ) );
            this->place( other.size() );
        }

        return *this;
    }

    template<typename T, size_t N>
    Buffer( Array<T, N> ) -> Buffer<T>;

    template<typename T, size_t N>
    Buffer( T( & )[N] ) -> Buffer<T>;
} // namespace SOMA

#endif // BUFFER_H_
