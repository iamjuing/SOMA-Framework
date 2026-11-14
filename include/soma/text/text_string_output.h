// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TEXT_STRING_OUTPUT_H_
#define SOMA_TEXT_STRING_OUTPUT_H_

#include "soma/text/string.h"
#include "soma/io/i_blocking_output_stream.h"
#include "soma/text/text_output.h"

namespace SOMA
{
    class Text_string_output: public Text_output
    {
    public:
        explicit Text_string_output( String& output_text );

    private:

        class Stream: public I_blocking_output_stream
        {
        public:
            explicit Stream( String& output_text );
            ~Stream() override = default;

            bool write( Buffer_view<uint8_t> data ) override;

        private:
            String& m_output_text;
        };

        Stream m_stream;
    };
} // namespace SOMA

#endif // TEXT_STRING_OUTPUT_H_
