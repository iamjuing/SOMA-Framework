// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_LINKED_LIST_H_
#define SOMA_LINKED_LIST_H_

#include <stdlib.h>

#include "soma/assert.h"

namespace SOMA
{

    ///
    /// @brief Templated class representing a linked list of objects of type T.
    ///        - in order the type T can be inserted into the Linked_list, it must contain at most one
    ///          member "Linked_list<T>::Item m_list_item".
    ///        - no dynamic memory allocation or specifying the maximum lenght of the list is needed
    ///        - the list can contain only objects of the same type (T)
    ///        - one object of type T can be inserted only in a single list
    ///        - API for traversing the list items (starting with the last inserted item)
    ///        - removing items is not supported
    ///
    ///          WARNING: This class is not thread-safe. The Linked_list is supposed to be filled in object creation phase,
    ///                   running in the same context.
    ///
    ///          Usage:
    ///                  My_class my_object1;
    ///                  My_class my_object2;
    ///                  My_class my_object3;
    ///
    ///                  Linked_list<My_class> m_list;
    ///
    ///                  m_list.insert(my_object1);
    ///                  m_list.insert(my_object2);
    ///                  m_list.insert(my_object3);
    ///
    ///                  for( auto& object: m_list )
    ///                  {
    ///                      object.my_method();
    ///                  }
    ///
    /// @param T type of objects that can be inserted into the list
    template<typename T>
    class Linked_list
    {
    public:

        ///
        /// @brief Inner class allowing objects of other classes to be inserted in the Linked_list.
        ///        - custom class T must contain a (private) member named "Linked_list<T>::Item m_list_item" to support list insertion
        ///        - can be inserted only in one Linked_list
        ///
        ///          Usage:
        ///                  class My_class
        ///                  {
        ///                  private:
        ///                      STATIC_LINKED_LIST_ITEM(My_class);
        ///                  }
        class Item
        {
        public:
            bool is_in_list( void );

        private:
            friend class Linked_list<T>;

            Item* mp_next = nullptr;

            ///
            /// @brief Pointer to the owning object.
            ///        Populated by the Linked_list on object insertion. Also used to detect whether the item is already inserted
            ///        in a Linked_list (which is a failure condition).
            T* mp_owner = nullptr;
        };

        ///
        /// @brief Internal iterator class to support "range-based for" loop traversing.
        class Iterator
        {
        public:
            explicit Iterator( Item* p_item );

            void operator++();
            T& operator*();
            bool operator!=( const Iterator& rhs ) const;

        private:
            Item* mp_iterator;
        };

        Linked_list( void ) = default;
        Linked_list( const Linked_list& list ) = delete;

        ~Linked_list( void );

        void clear( void );

        void insert( T& item );
        void insert_front( T& item );

        void remove( T& item );

        size_t size( void ) const;

        Iterator begin( void );
        Iterator begin( void ) const;

        Iterator end( void );
        Iterator end( void ) const;

        Linked_list& operator=( const Linked_list& ) = delete;

    private:
        Item* mp_first_item = nullptr;

        size_t m_item_cnt = 0;
    };

    ///
    /// @brief Ctor.
    /// @param p_item item the iterator will point to
    template<typename T>
    Linked_list<T>::Iterator::Iterator( Item* p_item )
        : mp_iterator( p_item )
    {
        // intentionally empty
    }

    ///
    /// @brief Moves the iterator to the next item.
    template<typename T>
    void Linked_list<T>::Iterator::operator++()
    {
        mp_iterator = mp_iterator->mp_next;
    }

    ///
    /// @brief Retrieves an object owning the current iterator item.
    ///        NOTE: C++ range based loop standard ensures this operator is never called if the iterator is at the end.
    ///              So, no need to check for nullptr in the client code before dereferencing.
    /// @retval object owning the current iterator item
    template<typename T>
    T& Linked_list<T>::Iterator::operator*()
    {
        return *( mp_iterator->mp_owner );
    }

    ///
    /// @brief Compares iterators.
    /// @retval true, if the iterators are equal
    template<typename T>
    bool Linked_list<T>::Iterator::operator!=( const Iterator& rhs ) const
    {
        return mp_iterator != rhs.mp_iterator;
    }

    ///
    /// @brief Retrieves whether the list item is already inserted in a list.
    /// @retval true, if inserted in a list
    ///         false otherwise
    template<typename T>
    bool Linked_list<T>::Item::is_in_list( void )
    {
        return ( mp_owner != nullptr );
    }

    template<typename T>
    Linked_list<T>::~Linked_list( void )
    {
        clear();
    }

    template<typename T>
    void Linked_list<T>::clear( void )
    {
        mp_first_item = nullptr;
        m_item_cnt = 0;
    }

    ///
    /// @brief Insert a new object to the back of the list.
    /// @param item object to be inserted
    template<typename T>
    void Linked_list<T>::insert( T& item )
    {
        // The content pointer is set on inserting to the List so, if it is already set, the item is already part of a List.
        ASSERT( item.m_list_item.mp_owner == nullptr );

        item.m_list_item.mp_owner = &item;

        if( mp_first_item )
        { // there is already a first item in the list, traverse the list until the end
            Item* p_next_item = mp_first_item;

            while( p_next_item->mp_next )
            {
                p_next_item = p_next_item->mp_next;
            }

            // queue item at the end of the list
            p_next_item->mp_next = &item.m_list_item;
        }
        else
        {
            // the item is the first of the list
            mp_first_item = &item.m_list_item;
        }

        ++m_item_cnt;
    }

    ///
    /// @brief Insert a new object to the front of the list.
    /// @param item object to be inserted
    template<typename T>
    void Linked_list<T>::insert_front( T& item )
    {
        // The content pointer is set on inserting to the List so, if it is already set, the item is already part of a List.
        ASSERT( item.m_list_item.mp_owner == nullptr );

        item.m_list_item.mp_owner = &item;

        if( mp_first_item )
        // not a first item in the list
        {
            item.m_list_item.mp_next = mp_first_item;
        }

        mp_first_item = &item.m_list_item;

        ++m_item_cnt;
    }

    template<typename T>
    void Linked_list<T>::remove( T& item )
    {
        // the item is not in any list
        ASSERT( item.m_list_item.mp_owner != nullptr );

        // remove the item from the list
        Item* p_prev_item = nullptr;
        Item* p_next_item = mp_first_item;

        bool found = false;

        while( p_next_item )
        {
            if( p_next_item == &item.m_list_item )
            {
                found = true;
                break;
            }

            p_prev_item = p_next_item;
            p_next_item = p_next_item->mp_next;
        }

        ASSERT( found == true );

        if( found )
        {
            if( p_prev_item )
            {
                p_prev_item->mp_next = p_next_item->mp_next;
            }
            else
            {
                mp_first_item = p_next_item->mp_next;
            }

            item.m_list_item.mp_owner = nullptr;

            --m_item_cnt;
        }
    }


    ///
    /// @brief Retrieves the number of elements.
    /// @retval number of elements
    template<typename T>
    size_t Linked_list<T>::size( void ) const
    {
        return m_item_cnt;
    }

    ///
    /// @brief Retrieves an iterator pointing to the first list item.
    ///        NOTE: This method is part of the range-based for loop support.
    /// @retval iterator pointing to the first list item
    template<typename T>
    typename Linked_list<T>::Iterator Linked_list<T>::begin( void )
    {
        return Iterator( mp_first_item );
    }

    template<typename T>
    typename Linked_list<T>::Iterator Linked_list<T>::begin( void ) const
    {
        return Iterator( mp_first_item );
    }

    ///
    /// @brief Retrieves iterator pointing to the end of the list (nullptr).
    ///        NOTE: This method is part of the range-based for loop support.
    /// @retval iterator pointing to the end of the list
    template<typename T>
    typename Linked_list<T>::Iterator Linked_list<T>::end( void )
    {
        return Iterator( nullptr );
    }

    template<typename T>
    typename Linked_list<T>::Iterator Linked_list<T>::end( void ) const
    {
        return Iterator( nullptr );
    }

    ///
    /// @brief Macro to simplify checking the calling object is inserted in a list.
    /// @retval true, if the object is inserted in a list
#define IS_IN_STATIC_LIST() this->m_list_item.is_in_list()

    ///
    /// @brief Macro to simplify making classes linked list insertable
    /// @param type class name
#define STATIC_LINKED_LIST_ITEM( type )      \
        Linked_list<type>::Item m_list_item; \
        friend class Linked_list<type>
} // namespace SOMA

#endif // LINKED_LIST_H_
