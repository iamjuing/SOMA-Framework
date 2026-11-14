// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SINGLE_PRODUCER_QUEUE_H_
#define SOMA_SINGLE_PRODUCER_QUEUE_H_

#include "soma/containers/i_queue.h"

namespace SOMA
{
    template<typename T>
    class I_single_producer_queue: public I_queue<T>
    {
    public:
        // zero-copy interface

        // NOTE: Not the entire free space buffer is necessarily returned (because of queue wrap around).
        //       Need to call multiple times.
        virtual Buffer<T> get_space( void ) const = 0;
        virtual void place( size_t items_cnt ) = 0;
    };
} // namespace SOMA

#endif // SINGLE_PRODUCER_QUEUE_H_
