// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BLOCKING_OUTPUT_STREAM_H
#define SOMA_BLOCKING_OUTPUT_STREAM_H

#include "soma/observer_handler.h"
#include "soma/io/i_blocking_output_stream.h"
#include "soma/runtime/synchronization_semaphore.h"

namespace SOMA
{
    class I_output_stream;

    class Blocking_output_stream: public I_blocking_output_stream
    {
    public:
        Blocking_output_stream( void ) = default;
        explicit Blocking_output_stream( I_output_stream& output_stream );
        Blocking_output_stream( const Blocking_output_stream& ) = delete;
        ~Blocking_output_stream() override;

        bool write( Buffer_view<uint8_t> data ) override;

        void subscribe( I_output_stream& output_stream );
        void unsubscribe( void );

    private:
        I_output_stream* mp_output_stream = nullptr;

        Synchronization_semaphore m_synchronization_semaphore;

        Observer_handler<Blocking_output_stream> m_observer_handler_stream_ready { *this,
                                                                                   &Blocking_output_stream::
                                                                                       async_callback_stream_ready };

        void async_callback_stream_ready( void );
    };
} // namespace SOMA

#endif // BLOCKING_OUTPUT_STREAM_H
