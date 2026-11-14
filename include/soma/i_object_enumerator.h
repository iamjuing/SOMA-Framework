// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_OBJECT_ENUMERATOR_H_
#define SOMA_I_OBJECT_ENUMERATOR_H_

#include "soma/text/string_view.h"
#include "soma/assert.h"

namespace SOMA
{

    ///
    /// @brief Templated interface class implementing automatic object enumerator. Whenever an object is created, it is added to the
    ///        linked list so the application code can travers all of them w/o knowing them or their count (can be used for example
    ///        for automatic initialization, logging etc.).
    ///        - not thread-safe
    ///        - only suitable for objects with endless life cycle (no removing mechanism implemented)
    ///        - construction is allowed only before the application is launched
    ///        - supports range based loop traversal
    ///        - objects are traversed in the order of creation
    ///          NOTE: The order shall be considered undefined if the objects' creation order is undefined (e.g. global objects
    ///                created in different translation units).
    ///
    ///        Usage example:
    ///
    ///        class My_class : public I_object_enumerator<My_class>
    ///        {
    ///        public:
    ///            void method( void );
    ///
    ///        private:
    ///            int value;
    ///        }
    ///
    ///        My_class my_object1;
    ///        My_class my_object2;
    ///        My_class my_object3;
    ///
    ///        int main()
    ///        {
    ///            // traversing all objects of My_class (range-based for loop)
    ///
    ///            for (My_class& object: My_class::objects() )
    ///            {
    ///                object.method();
    ///            }
    ///        }
    ///
    /// @param T type of the enumerated class
    template<typename T>
    class I_object_enumerator
    {
    public:

        ///
        /// @brief Internal iterator class to support "range-based for" loop traversing.
        class Iterator
        {
        public:
            explicit Iterator( T* p_object );

            void operator++();
            T& operator*();
            bool operator!=( const Iterator& rhs ) const;

            Iterator begin( void );
            Iterator end( void );

        private:
            T* mp_iterator;
        };

        I_object_enumerator( void );
        ~I_object_enumerator( void );

        I_object_enumerator( const I_object_enumerator& ) = delete;
        I_object_enumerator& operator=( const I_object_enumerator& ) = delete;
        I_object_enumerator( I_object_enumerator&& ) = delete;
        I_object_enumerator& operator=( I_object_enumerator&& ) = delete;

        static Iterator objects( void );

        static bool present( void );

        static T* find_object( String_view name );

    private:
        static inline T* mp_first_object = nullptr;

        T* mp_next_object = nullptr;
    };

    ///
    /// @brief Ctor.
    /// @param p_object the iterator will point to
    template<typename T>
    I_object_enumerator<T>::Iterator::Iterator( T* p_object )
        : mp_iterator( p_object )
    {
        // intentionally empty
    }

    ///
    /// @brief Moves the iterator to the next objects.
    template<typename T>
    void I_object_enumerator<T>::Iterator::operator++()
    {
        mp_iterator = mp_iterator->I_object_enumerator<T>::mp_next_object;
    }

    ///
    /// @brief Retrieves the object the iterator points to.
    /// @retval object the iterator points to
    template<typename T>
    T& I_object_enumerator<T>::Iterator::operator*()
    {
        return *mp_iterator;
    }

    ///
    /// @brief Compares iterators.
    /// @retval true, if the iterators are equal
    template<typename T>
    bool I_object_enumerator<T>::Iterator::operator!=( const Iterator& rhs ) const
    {
        return mp_iterator != rhs.mp_iterator;
    }

    ///
    /// @brief Retrieves an iterator pointing to the first object.
    ///        NOTE: This method is part of the range-based for loop support.
    /// @retval iterator pointing to the first object
    template<typename T>
    typename I_object_enumerator<T>::Iterator I_object_enumerator<T>::Iterator::begin( void )
    {
        return Iterator( mp_first_object );
    }

    ///
    /// @brief Retrieves iterator pointing to the end of the list (nullptr).
    ///        NOTE: This method is part of the range-based for loop support.
    /// @retval iterator pointing to the end of the list
    template<typename T>
    typename I_object_enumerator<T>::Iterator I_object_enumerator<T>::Iterator::end( void )
    {
        return Iterator( nullptr );
    }

    ///
    /// @brief Ctor
    ///        Adds the newly created object to the linked list.
    template<typename T>
    I_object_enumerator<T>::I_object_enumerator( void )
    {
        extern bool is_application_launched( void );

        ASSERT( is_application_launched() == false );

        if( mp_first_object )
        // there is already a an item in the list, traverse the list until the end
        {
            T* p_item = mp_first_object;

            while( p_item->I_object_enumerator<T>::mp_next_object )
            {
                p_item = p_item->I_object_enumerator<T>::mp_next_object;
            }

            // queue item at the end of the list
            p_item->I_object_enumerator<T>::mp_next_object = static_cast<T*>( this );
        }
        else
        // first item
        {
            mp_first_object = static_cast<T*>( this );
        }
    }

    ///
    /// @brief Dtor
    ///        If the objects are deallocating, the list needs to be cleared as it is based on static variable.
    ///        So, the next object list construction starts properly from scratch.
    ///        NOTE: It is assumed that all the objects in the list are being deallocated as the framework does not
    ///              support run-time reconfiguration and object creation.
    ///              All objects must be allocated during Application containter construction, and de-allocated
    ///              during the Application container destruction.
    template<typename T>
    I_object_enumerator<T>::~I_object_enumerator( void )
    {
        // no need to travers the list, just disconnect the static variable from it
        mp_first_object = nullptr;
    }

    ///
    /// @brief Retrieves iterator for the first object in the list.
    ///        This is supposed to be called in the range-based for loop that requires an object
    ///        supporting .begin() and .end() methods.
    template<typename T>
    typename I_object_enumerator<T>::Iterator I_object_enumerator<T>::objects( void )
    {
        return Iterator( mp_first_object );
    }

    ///
    /// @brief Retrieves whether there is at least one object in the list.
    /// @retval true, if there is at least one object in the list
    ///         false otherwise
    template<typename T>
    bool I_object_enumerator<T>::present( void )
    {
        return ( mp_first_object != nullptr );
    }

    ///
    /// @brief Finds an enumerated object by name.
    /// @retval Pointer to the matching object, or nullptr when it is absent.
    template<typename T>
    T* I_object_enumerator<T>::find_object( String_view name )
    {
        T* rv_object = nullptr;

        if( name.is_empty() == false )
        {
            for( auto& object: T::objects() )
            {
                if( name == object.name() )
                {
                    rv_object = &object;
                    break;
                }
            }
        }

        return rv_object;
    }
} // namespace SOMA

#endif // I_OBJECT_ENUMERATOR_H_
