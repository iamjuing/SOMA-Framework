// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SINGLE_CONSUMER_QUEUE_H_
#define SOMA_SINGLE_CONSUMER_QUEUE_H_

#include "soma/containers/i_queue.h"

namespace SOMA
{
    template<typename T>
    class I_single_consumer_queue: public I_queue<T>
    {
    public:
        // zero-copy interface

        // NOTE: The returned buffer does not necessarily contain all items available (because of queue wrap around).
        //       Need to call multiple times until empty buffer is returned.
        virtual Buffer_view<T> view_items( void ) const = 0;
        virtual void displace( size_t items_cnt ) = 0;
    };
} // namespace SOMA

#endif // SINGLE_CONSUMER_QUEUE_H_
