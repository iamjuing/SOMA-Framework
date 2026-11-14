// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BUFFERED_INPUT_STREAM_H_
#define SOMA_BUFFERED_INPUT_STREAM_H_

#include "soma/io/i_input_stream.h"
#include "soma/observer_handler.h"
#include "soma/containers/ring_buffer.h"

namespace SOMA
{
    class Buffered_input_stream: public I_input_stream
    {
    public:
        explicit Buffered_input_stream( String_view name,
                                        I_input_stream& input_stream,
                                        Ring_buffer<uint8_t>& buffer );
        ~Buffered_input_stream() override;

        Result_e read( Buffer<uint8_t>& out_buffer ) override;
        void cancel_read( void ) override;

    private:
        I_input_stream& m_input_stream;
        Ring_buffer<uint8_t>& m_ring_buffer;
        Buffer<uint8_t> m_transfer_buffer;
        Buffer<uint8_t>* mp_upper_layer_buffer = nullptr;

        Observer_handler<Buffered_input_stream> m_observer_stream_ready { *this,
                                                                          &Buffered_input_stream::async_callback_stream_ready };

        void async_callback_stream_ready( void );

        void start_reading( void );
    };
} // namespace SOMA

#endif // BUFFERED_INPUT_STREAM_H_
