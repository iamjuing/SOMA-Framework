// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/text/text_string_output.h"

namespace SOMA
{
    Text_string_output::Text_string_output( String& output_text )
        : Text_output( m_stream ),
          m_stream( output_text )
    {
        // intentionally empty
    }

    Text_string_output::Stream::Stream( String& output_text )
        : m_output_text( output_text )
    {
        // intentionally empty
    }

    bool Text_string_output::Stream::write( Buffer_view<uint8_t> data )
    {
        const bool rv_success = data.size() <= m_output_text.space();

        m_output_text += String_view( data );

        return rv_success;
    }
} // namespace SOMA
