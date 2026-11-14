// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_REVERSE_ITERATOR_H_
#define SOMA_REVERSE_ITERATOR_H_

namespace SOMA
{
    template<typename T>
    class Reverse_iterator
    {
    public:

        template<typename U>
        class Iterator
        {
        public:
            explicit constexpr Iterator( U it )
                : m_it( it ) {}

            constexpr Iterator& operator++(){ --m_it; return *this; }

            constexpr auto& operator*() const { return *m_it; }

            constexpr bool operator==( const Iterator& rhs ) const { return m_it == rhs.m_it; }
            constexpr bool operator!=( const Iterator& rhs ) const { return m_it != rhs.m_it; }

        private:
            U m_it;
        };

        explicit constexpr Reverse_iterator( T& iterable )
            : m_iterable( iterable )
        {
            // intentionally empty
        }

        constexpr auto begin() const
        {
            auto it = m_iterable.end(); // element past the last element
            --it; // get to the last element

            return Iterator( it );
        }

        constexpr auto end() const
        {
            auto it = m_iterable.begin(); // first element in the container
            --it; // element before the first element

            return Iterator( it );
        }

    private:
        T& m_iterable;
    };
} // namespace SOMA

#endif // REVERSE_ITERATOR_H_
