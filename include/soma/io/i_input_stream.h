// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_INPUT_STREAM_H_
#define SOMA_I_INPUT_STREAM_H_

#include <stdint.h>
#include <stddef.h>

#include "soma/containers/linked_list.h"
#include "soma/i_observable.h"
#include "soma/containers/buffer.h"
#include "soma/i_object_enumerator.h"
#include "soma/text/string_view.h"
#include "soma/text/text_output.h"

namespace SOMA
{

    class I_input_stream: public I_observable, public I_object_enumerator<I_input_stream>
    {
    public:
        explicit I_input_stream( String_view name );
        virtual ~I_input_stream() = default;

        enum class Result_e
        {
            ready,
            wait,
        };

        ///
        /// @brief Reads data from the input stream in a non-blocking manner.
        /// @param out_buffer  buffer to fill with data
        /// @retval ready, if data was read and the stream is ready for another read call
        ///         wait, the caller will be notified:
        ///               - when (some) data is ready in the buffer (not necessarily the full buffer)
        ///               - when the stream becomes unavailable
        ///
        /// NOTE: When "wait" result is returned, the out_buffer must stay valid until the read operation completes
        ///       or until cancel_read() returns.
        ///
        /// WARNING: If the stream is unavailable at the time of calling read(), "wait" is returned.
        ///          This allows to wait for data on a stream that is not yet available.
        virtual Result_e read( Buffer<uint8_t>& out_buffer ) = 0;

        ///
        /// @brief Cancels a pending read operation, if there is any.
        ///
        /// After this call returns, the stream must not access the buffer passed to the cancelled read().
        virtual void cancel_read( void ) = 0;

        String_view name( void ) const;
        void to_text( Text_output& output ) const;

    private:
        STATIC_LINKED_LIST_ITEM( I_input_stream );

        String_view m_name;
    };

    inline I_input_stream::I_input_stream( String_view name )
        : m_name( name )
    {
        // intentionally empty
    }

    inline String_view I_input_stream::name( void ) const
    {
        return m_name;
    }

    inline void I_input_stream::to_text( Text_output& output ) const
    {
        output << m_name;
    }
} // namespace SOMA

#endif // I_INPUT_STREAM_H_
