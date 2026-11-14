// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_ARRAY_H_
#define SOMA_ARRAY_H_

#include <cstddef>
#include <algorithm>
#include <limits>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include "soma/assert.h"

namespace SOMA
{
    inline constexpr size_t dynamic_extent = std::numeric_limits<size_t>::max();

    ///
    /// @brief A non-owning array view or an array with owned storage.
    ///
    /// Array<T> is a pointer-and-size view. Array<T, N> owns N value-initialized
    /// elements and derives from Array<T>, so both forms can be passed to a
    /// function accepting Array<T> (or Array<const T> for read-only access).
    /// The backing storage must outlive any view of it.
    ///
    /// @code
    /// #include <cstdint>
    /// using SOMA::Array;
    /// void array_examples()
    /// {
    ///     uint8_t external[] { 1, 2, 3 };
    ///     Array<uint8_t> empty_view;                       // Empty, with no storage.
    ///     Array<uint8_t> view { external };                // View; size deduced from C array.
    ///     Array inferred_view { external };                // Also deduces Array<uint8_t>.
    ///     Array<uint8_t> first_two { external, 2u };       // View of part of the storage.
    ///     Array<uint8_t> tail = view.sub_array( 1u );      // Another view; size 2.
    ///
    ///     Array<uint8_t, 3u> owned;                  // Owns three zero-initialized uint8_t values.
    ///     Array<uint8_t, 0u> empty_owned;            // Owns no elements.
    ///     Array values { uint8_t{10}, uint8_t{20}, uint8_t{30} }; // Deduces Array<uint8_t, 3>.
    ///     Array<uint8_t, 3u> copy { owned };         // Copies elements into new storage.
    ///     Array moved = std::move( copy );           // Moves elements into new storage.
    ///     owned = { 4, 5, 6 };                       // Replaces all three elements.
    ///     Array<uint8_t> owned_view { owned };       // Views owned storage without copying.
    ///     Array<const uint8_t> read_only { owned };  // Read-only view of owned storage.
    ///     Array owned_views { owned };               // Owns one view of owned storage.
    ///     Array copied = owned;                      // Copy-initialization copies storage.
    ///
    ///     Array one_view { view };                   // Owns one Array<uint8_t> view.
    ///     Array two_views { view, tail };            // Owns two Array<uint8_t> views.
    ///     Array<uint8_t> copied_view { view };       // Copies the view itself.
    ///
    ///     auto consume = []( Array<const uint8_t> values ) { (void)values; };
    ///     consume( owned );
    ///     consume( view );
    ///     owned.fill( 7 );
    ///     size_t count = owned.size();
    ///     size_t bytes = owned.size_B();
    ///     uint8_t first = owned[0];
    ///     uint8_t* storage = owned.storage();
    ///     for( uint8_t value: owned ) { (void)value; }
    ///     (void)count; (void)bytes; (void)first; (void)storage; (void)moved;
    /// }
    /// @endcode
    ///
    /// An owning constructor requires exactly N arguments at compile time.
    /// A brace-list assignment checks its item count with ASSERT at runtime.
    template<typename T, size_t N = dynamic_extent>
    class Array;

    template<typename T>
    class Array<T, dynamic_extent>
    {
    public:
        using Iterator_t = T*;

        constexpr Array();
        constexpr Array( T* p_buffer,
                         size_t size );
        constexpr Array( const Array& array );
        template<typename U, size_t M,
                 typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value &&
                                             std::is_convertible<U*, T*>::value&&
                                             ( !std::is_same<T, U>::value || M != dynamic_extent )>>
        constexpr Array( const Array<U, M>& array );
        template<size_t N>
        constexpr Array( T ( & p_buffer )[N] );

        constexpr Array& operator=( const Array& str );

        constexpr size_t size( void ) const;
        constexpr size_t size_B( void ) const;

        constexpr T& operator[]( std::size_t idx );
        constexpr const T& operator[]( std::size_t idx ) const;

        template<typename U, size_t M>
        constexpr bool operator==( const Array<U, M>& rhs ) const;

        constexpr Iterator_t begin() const;
        constexpr Iterator_t end() const;

        constexpr void fill( T value );

        constexpr Array<T> sub_array( size_t first_item_idx,
                                      size_t item_cnt = std::numeric_limits<size_t>::max() ) const;

        constexpr T* storage( void ) const;

    private:
        T* mp_storage = nullptr;
        size_t m_size = 0u;
    };

    template<typename T, size_t N>
    class Array: public Array<T, dynamic_extent>
    {
    public:
        static_assert( N > 0u, "Zero extent uses Array<T, 0>" );

        constexpr Array();

        template<typename ... Items,
                 typename = std::enable_if_t<( sizeof...( Items ) == N ) && ( N > 0u ) &&
                                             ( std::is_constructible_v<T, Items&&> && ... )>>
        constexpr Array( Items&& ... items );

        constexpr Array( const Array& other );

        constexpr Array( Array&& other ) noexcept( std::is_nothrow_move_constructible_v<T> );

        constexpr Array& operator=( const Array& other );

        constexpr Array& operator=( Array&& other ) noexcept( std::is_nothrow_move_assignable_v<T> );

        constexpr Array& operator=( std::initializer_list<T> items );

    private:
        using View = Array<T, dynamic_extent>;

        template<size_t... Indices>
        constexpr Array( const Array& other,
                         std::index_sequence<Indices...> );

        template<size_t... Indices>
        constexpr Array( Array&& other,
                         std::index_sequence<Indices...> )
        noexcept( std::is_nothrow_move_constructible_v<T> );

        constexpr void bind_storage();

        T m_array[N];
    };

    template<typename T>
    class Array<T, 0u>: public Array<T, dynamic_extent>
    {
    public:
        constexpr Array();
        constexpr Array( const Array& other );
        constexpr Array( Array&& other ) noexcept;

        constexpr Array& operator=( const Array& other );
        constexpr Array& operator=( Array&& other ) noexcept;
        constexpr Array& operator=( std::initializer_list<T> items );

    private:
        using View = Array<T, dynamic_extent>;
    };

    template<typename T>
    constexpr Array<T, dynamic_extent>::Array() = default;

    template<typename T>
    constexpr Array<T, dynamic_extent>::Array( T* p_buffer,
                                               size_t size )
        : mp_storage( p_buffer ),
          m_size( size )
    {
        ASSERT( ( size == 0u ) ||
                ( ( size > 0u ) && ( mp_storage != nullptr ) ) );
    }

    template<typename T>
    constexpr Array<T, dynamic_extent>::Array( const Array<T>& array )
        : mp_storage( array.mp_storage ),
          m_size( array.m_size )
    {
    }

    template<typename T>
    template<typename U, size_t M, typename>
    constexpr Array<T, dynamic_extent>::Array( const Array<U, M>& array )
        : mp_storage( array.storage() ),
          m_size( array.size() )
    {
    }

    template<typename T>
    template<size_t N>
    constexpr Array<T, dynamic_extent>::Array( T ( & p_buffer )[N] )
        : Array( p_buffer, N )
    {
    }

    template<typename T>
    constexpr Array<T, dynamic_extent>& Array<T, dynamic_extent>::operator=( const Array& str ) = default;

    template<typename T>
    constexpr size_t Array<T, dynamic_extent>::size( void ) const
    {
        return m_size;
    };

    template<typename T>
    constexpr size_t Array<T, dynamic_extent>::size_B( void ) const
    {
        return size() * sizeof( T );
    }

    template<typename T>
    constexpr T& Array<T, dynamic_extent>::operator[]( std::size_t idx )
    {
        ASSERT( idx < m_size );

        return mp_storage[idx];
    }

    template<typename T>
    constexpr const T& Array<T, dynamic_extent>::operator[]( std::size_t idx ) const
    {
        ASSERT( idx < m_size );

        return mp_storage[idx];
    }

    template<typename T>
    template<typename U, size_t M>
    constexpr bool Array<T, dynamic_extent>::operator==( const Array<U, M>& rhs ) const
    {
        static_assert( std::is_same<std::remove_const_t<T>, std::remove_const_t<U>>::value,
                       "Array element types must match" );

        bool rv_equal = false;

        if( m_size == rhs.size() )
        {
            rv_equal = true;

            for( size_t index = 0u; ( index < m_size ) && rv_equal; ++index )
            {
                rv_equal = ( mp_storage[index] == rhs[index] );
            }
        }

        return rv_equal;
    }

    template<typename T>
    constexpr typename Array<T, dynamic_extent>::Iterator_t Array<T, dynamic_extent>::begin() const
    {
        return mp_storage;
    }

    template<typename T>
    constexpr typename Array<T, dynamic_extent>::Iterator_t Array<T, dynamic_extent>::end() const
    {
        return m_size == 0u ? mp_storage : mp_storage + m_size;
    }

    template<typename T>
    constexpr T* Array<T, dynamic_extent>::storage( void ) const
    {
        return mp_storage;
    }

    template<typename T>
    constexpr Array<T> Array<T, dynamic_extent>::sub_array( size_t first_item_idx,
                                                            size_t item_cnt ) const
    {
        Array<T> rv_sub_array;

        ASSERT( first_item_idx <= m_size );

        if( first_item_idx < m_size )
        {
            item_cnt = std::min( item_cnt, m_size - first_item_idx );

            rv_sub_array = Array( &mp_storage[first_item_idx], item_cnt );
        }

        return rv_sub_array;
    }

    template<typename T>
    constexpr void Array<T, dynamic_extent>::fill( T value )
    {
        std::fill( begin(), end(), value );
    }

    template<typename T, size_t N>
    constexpr Array<T, N>::Array()
        : m_array{}
    {
        bind_storage();
    }

    template<typename T, size_t N>
    template<typename ... Items, typename>
    constexpr Array<T, N>::Array( Items&& ... items )
        : m_array{ T( std::forward<Items>( items ) )... }
    {
        bind_storage();
    }

    template<typename T, size_t N>
    constexpr Array<T, N>::Array( const Array& other )
        : Array( other, std::make_index_sequence<N>{} )
    {
    }

    template<typename T, size_t N>
    constexpr Array<T, N>::Array( Array&& other ) noexcept( std::is_nothrow_move_constructible_v<T> )
        : Array( std::move( other ), std::make_index_sequence<N>{} )
    {
    }

    template<typename T, size_t N>
    template<size_t... Indices>
    constexpr Array<T, N>::Array( const Array& other,
                                  std::index_sequence<Indices...> )
        : View(),
          m_array{ other.m_array[Indices] ... }
    {
        bind_storage();
    }

    template<typename T, size_t N>
    template<size_t... Indices>
    constexpr Array<T, N>::Array( Array&& other,
                                  std::index_sequence<Indices...> )
    noexcept( std::is_nothrow_move_constructible_v<T> )
        : View(),
          m_array{ std::move( other.m_array[Indices] )... }
    {
        bind_storage();
    }

    template<typename T, size_t N>
    constexpr Array<T, N>& Array<T, N>::operator=( const Array& other )
    {
        for( size_t index = 0u; index < N; ++index )
        {
            m_array[index] = other.m_array[index];
        }
        bind_storage();

        return *this;
    }

    template<typename T, size_t N>
    constexpr Array<T, N>& Array<T, N>::operator=( Array&& other ) noexcept( std::is_nothrow_move_assignable_v<T> )
    {
        for( size_t index = 0u; index < N; ++index )
        {
            m_array[index] = std::move( other.m_array[index] );
        }
        bind_storage();

        return *this;
    }

    template<typename T, size_t N>
    constexpr Array<T, N>& Array<T, N>::operator=( std::initializer_list<T> items )
    {
        ASSERT( items.size() == N );
        std::copy( items.begin(), items.end(), m_array );
        bind_storage();

        return *this;
    }

    template<typename T, size_t N>
    constexpr void Array<T, N>::bind_storage()
    {
        View::operator=( View { m_array, N } );
    }

    template<typename T>
    constexpr Array<T, 0u>::Array()
        : View()
    {
    }

    template<typename T>
    constexpr Array<T, 0u>::Array( const Array& other )
        : View()
    {
        (void)other;
    }

    template<typename T>
    constexpr Array<T, 0u>::Array( Array&& other ) noexcept
        : View()
    {
        (void)other;
    }

    template<typename T>
    constexpr Array<T, 0u>& Array<T, 0u>::operator=( const Array& other )
    {
        (void)other;
        View::operator=( View{} );

        return *this;
    }

    template<typename T>
    constexpr Array<T, 0u>& Array<T, 0u>::operator=( Array&& other ) noexcept
    {
        (void)other;
        View::operator=( View{} );

        return *this;
    }

    template<typename T>
    constexpr Array<T, 0u>& Array<T, 0u>::operator=( std::initializer_list<T> items )
    {
        ASSERT( items.size() == 0u );
        View::operator=( View{} );

        return *this;
    }

    template<typename T, size_t N>
    Array( T( & )[N] ) -> Array<T>;

    template<typename T>
    Array( T*, size_t ) -> Array<T>;

    template<typename T>
    Array( Array<T> ) -> Array<Array<T>, 1u>;

    template<typename T, size_t N, typename = std::enable_if_t<N != dynamic_extent>>
    explicit Array( Array<T, N>& ) -> Array<Array<T>, 1u>;

    template<typename T, size_t N, typename = std::enable_if_t<N != dynamic_extent>>
    explicit Array( const Array<T, N>& ) -> Array<Array<T>, 1u>;

    template<typename T, size_t N, size_t M, size_t... Ms>
    Array( const Array<T, N>&, const Array<T, M>&, const Array<T, Ms>&... )
    -> Array<Array<T>, 2u + sizeof...( Ms )>;

    template<typename T, typename ... U,
             typename = std::enable_if_t<( std::is_same_v<T, U> && ... )>>
    Array( T, U ... ) -> Array<T, 1u + sizeof...( U )>;
} // namespace SOMA

#endif // ARRAY_H_
