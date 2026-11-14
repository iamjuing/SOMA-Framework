// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_QUEUE_H_
#define SOMA_I_QUEUE_H_

#include  "soma/containers/buffer.h"

namespace SOMA
{
    class Signal;

    template<typename T>
    class I_queue
    {
    public:
        virtual size_t capacity( void ) const = 0;

        virtual bool is_empty( void ) const = 0;
        virtual bool is_full( void ) const = 0;

        virtual size_t size( void ) const = 0;
        virtual size_t space( void ) const = 0;

        // copy interface
        virtual bool push( const T& item ) = 0;
        virtual size_t push( Buffer_view<T> data ) = 0;

        virtual bool pop( T& out_item ) = 0;
        virtual size_t pop( Buffer<T>& out_buffer ) = 0;

        // state signalling interface
        virtual Signal& signal_queue_empty( void ) = 0;      // on pop() of the last item
        virtual Signal& signal_space_available( void ) = 0;  // on pop() from a full queue

        virtual Signal& signal_queue_full( void ) = 0;       // on push() to a full queue
        virtual Signal& signal_items_available( void ) = 0;  // on push() to an empty queue
    };
} // namespace SOMA

#endif // I_QUEUE_H_
