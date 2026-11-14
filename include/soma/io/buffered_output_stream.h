// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_BUFFERED_OUTPUT_STREAM_H_
#define SOMA_BUFFERED_OUTPUT_STREAM_H_

#include "soma/io/i_output_stream.h"
#include "soma/observer_handler.h"
#include "soma/containers/ring_buffer.h"

namespace SOMA
{
    class Buffered_output_stream: public I_output_stream
    {
    public:
        explicit Buffered_output_stream( String_view name,
                                         I_output_stream& output_stream,
                                         Ring_buffer<uint8_t>& buffer );
        ~Buffered_output_stream() override;

        Result_e write( Buffer_view<uint8_t> data ) override;

    private:
        I_output_stream& m_output_stream;
        Ring_buffer<uint8_t>& m_ring_buffer;
        size_t m_bytes_in_transfer = 0u;
        Buffer_view<uint8_t> m_upper_layer_buffer;

        Observer_handler<Buffered_output_stream> m_observer_stream_ready { *this,
                                                                           &Buffered_output_stream::async_callback_stream_ready };

        void async_callback_stream_ready( void );

        bool start_writing( void );
    };
} // namespace SOMA

#endif // BUFFERED_OUTPUT_STREAM_H_
