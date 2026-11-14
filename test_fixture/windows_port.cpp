// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/include/FreeRTOS.h"

static bool is_in_ISR = false;

void set_ISR_context( bool is_ISR )
{
    is_in_ISR = is_ISR;
}

BaseType_t xPortIsInsideInterrupt( void )
{
    // TODO does this have an effect in Windows port?

    return is_in_ISR;
}
