// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_INTERNAL_PORT_TRAITS_H_
#define SOMA_INTERNAL_PORT_TRAITS_H_

#include <stdint.h>

#include "soma/config.h"

namespace SOMA
{
    namespace Internal
    {
#if SOMA_CONFIG_TICK_TYPE_WIDTH_BITS == 16
        using Port_tick_t = uint16_t;
#elif SOMA_CONFIG_TICK_TYPE_WIDTH_BITS == 32
        using Port_tick_t = uint32_t;
#else
        using Port_tick_t = uint64_t;

#endif

        inline constexpr uint32_t c_tick_rate_hz = SOMA_CONFIG_TICK_RATE_HZ;
    } // namespace Internal
} // namespace SOMA

#endif // SOMA_INTERNAL_PORT_TRAITS_H_
