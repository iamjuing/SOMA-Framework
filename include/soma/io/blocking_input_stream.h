// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BLOCKING_INPUT_STREAM_H
#define SOMA_BLOCKING_INPUT_STREAM_H

#include "soma/observer_handler.h"
#include "soma/io/i_blocking_input_stream.h"
#include "soma/runtime/synchronization_semaphore.h"

namespace SOMA
{
    class I_input_stream;

    class Blocking_input_stream: public I_blocking_input_stream
    {
    public:
        explicit Blocking_input_stream( I_input_stream& input_stream );
        Blocking_input_stream( const Blocking_input_stream& ) = delete;
        ~Blocking_input_stream() override;

        void read( Buffer<uint8_t>& out_buffer ) override;
        void read( Buffer<uint8_t>& out_buffer,
                   uint32_t timeout_ms ) override;

    private:
        I_input_stream& m_input_stream;

        Synchronization_semaphore m_synchronization_semaphore;

        Observer_handler<Blocking_input_stream> m_observer_stream_ready { *this,
                                                                          &Blocking_input_stream::async_callback_stream_ready };

        void async_callback_stream_ready( void );
    };
} // namespace SOMA

#endif // BLOCKING_INPUT_STREAM_H
