// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_NULL_OUTPUT_STREAM_H_
#define SOMA_NULL_OUTPUT_STREAM_H_

#include "soma/io/i_output_stream.h"

namespace SOMA
{
    class Null_output_stream: public I_output_stream
    {
    public:
        explicit Null_output_stream( String_view name = "null" )
            : I_output_stream( name )
        {
        }

        Result_e write( [[maybe_unused]] Buffer_view<uint8_t> data ) override
        {
            return Result_e::ready;
        }
    };
} // namespace SOMA

#endif // NULL_OUTPUT_STREAM_H_
