// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_OUTPUT_STREAM_H_
#define SOMA_I_OUTPUT_STREAM_H_

#include <stdint.h>
#include <stddef.h>

#include "soma/containers/linked_list.h"
#include "soma/i_observable.h"
#include "soma/containers/buffer_view.h"
#include "soma/text/string_view.h"
#include "soma/text/text_output.h"
#include "soma/i_object_enumerator.h"

namespace SOMA
{

    class I_output_stream: public I_observable, public I_object_enumerator<I_output_stream>
    {
    public:
        explicit I_output_stream( String_view name );
        virtual ~I_output_stream() = default;

        enum class Result_e
        {
            ready,
            wait,
            unavailable,
        };

        ///
        /// @brief Writes data to the output stream in a non-blocking manner.
        /// @param data  data to write
        /// @retval ready, if all the provided data was written and the stream is ready for another write call
        ///         wait, the caller will be notified when all the data is written
        ///         unavailable, the output stream is not available for writing
        ///
        /// NOTE: When "wait" result is returned, the data must stay valid until the write operation completes.
        virtual Result_e write( Buffer_view<uint8_t> data ) = 0;

        String_view name( void ) const;
        void to_text( Text_output& output ) const;

    private:
        STATIC_LINKED_LIST_ITEM( I_output_stream );

        String_view m_name;
    };

    inline I_output_stream::I_output_stream( String_view name )
        : m_name( name )
    {
        // intentionally empty
    }

    inline String_view I_output_stream::name( void ) const
    {
        return m_name;
    }

    inline void I_output_stream::to_text( Text_output& output ) const
    {
        output << m_name;
    }
} // namespace SOMA

#endif // I_OUTPUT_STREAM_H_
