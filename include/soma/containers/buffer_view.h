// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BUFFER_VIEW_H_
#define SOMA_BUFFER_VIEW_H_

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <limits>

#include "soma/array.h"

namespace SOMA
{

    template<typename T>
    class Buffer_view
    {
    public:
        static_assert( !std::is_const_v<T>,
                       "Buffer_view element type must not be const" );

        using Iterator_t = typename Array<const T>::Iterator_t;

        constexpr Buffer_view( void ) = default;
        explicit constexpr Buffer_view( Array<const T> array );
        constexpr Buffer_view( const Buffer_view& buffer ) = default;

        constexpr Buffer_view& operator=( const Buffer_view& buffer ) = default;

        constexpr size_t size( void ) const;

        constexpr Iterator_t begin() const;
        constexpr Iterator_t end() const;

        constexpr Buffer_view<T> sub_buffer( size_t first_item_idx,
                                             size_t item_cnt = std::numeric_limits<size_t>::max() ) const;

        constexpr bool operator==( const Buffer_view<T>& rhs ) const;

        constexpr T operator[]( size_t index ) const;

        constexpr Array<const T> array( void ) const;

    private:
        Array<const T> m_array;
    };

    template<typename T>
    Buffer_view( Array<T> ) -> Buffer_view<std::remove_const_t<T>>;

    template<typename T>
    constexpr Buffer_view<T>::Buffer_view( Array<const T> array )
        : m_array( array )
    {
        // intentionally empty
    }

    template<typename T>
    constexpr size_t Buffer_view<T>::size( void ) const
    {
        return m_array.size();
    }

    template<typename T>
    constexpr typename Buffer_view<T>::Iterator_t Buffer_view<T>::begin() const
    {
        return m_array.begin();
    }

    template<typename T>
    constexpr typename Buffer_view<T>::Iterator_t Buffer_view<T>::end() const
    {
        return m_array.end();
    }

    template<typename T>
    constexpr Buffer_view<T> Buffer_view<T>::sub_buffer( size_t first_item_idx,
                                                         size_t item_cnt ) const
    {
        return Buffer_view( m_array.sub_array( first_item_idx, item_cnt ) );
    }

    template<typename T>
    constexpr bool Buffer_view<T>::operator==( const Buffer_view<T>& rhs ) const
    {
        return m_array == rhs.m_array;
    }

    template<typename T>
    constexpr Array<const T> Buffer_view<T>::array( void ) const
    {
        return m_array;
    }

    template<typename T>
    constexpr T Buffer_view<T>::operator[]( size_t index ) const
    {
        return m_array[index];
    }
} // namespace SOMA

#endif // BUFFER_VIEW_H_
