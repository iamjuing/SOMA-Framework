// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_BLOCKING_OUTPUT_STREAM_H
#define SOMA_I_BLOCKING_OUTPUT_STREAM_H

#include <cstdint>

#include "soma/containers/buffer_view.h"

namespace SOMA
{
    class I_blocking_output_stream
    {
    public:
        virtual ~I_blocking_output_stream() = default;

        virtual bool write( Buffer_view<uint8_t> data ) = 0;
    };
} // namespace SOMA

#endif // I_BLOCKING_OUTPUT_STREAM_H
