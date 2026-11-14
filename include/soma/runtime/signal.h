// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_SIGNAL_H_
#define SOMA_SIGNAL_H_

#include "soma/i_observable.h"

namespace SOMA
{
    class Signal: public I_observable
    {
    public:
        Signal( void ) = default;
        Signal( const Signal& ) = delete;
        Signal& operator=( const Signal& ) = delete;

        void emit( void )
        {
            notify();
        }
    };
} // namespace SOMA

#endif // SOMA_SIGNAL_H_
