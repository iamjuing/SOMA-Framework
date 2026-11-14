// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_BLOCKING_INPUT_STREAM_H
#define SOMA_I_BLOCKING_INPUT_STREAM_H

#include <cstdint>
#include <cstddef>

#include "soma/containers/buffer.h"
#include "soma/time.h"

namespace SOMA
{
    class I_blocking_input_stream
    {
    public:
        virtual ~I_blocking_input_stream() = default;

        virtual void read( Buffer<uint8_t>& out_buffer ) = 0;
        virtual void read( Buffer<uint8_t>& out_buffer,
                           uint32_t timeout_ms ) = 0;
    };
} // namespace SOMA

#endif // I_BLOCKING_INPUT_STREAM_H
