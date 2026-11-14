// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_MOCK_OBSERVER_H_
#define SOMA_MOCK_OBSERVER_H_

#include <gmock/gmock.h>

#include "soma/observer_handler.h"
#include "soma/i_observable.h"

class Mock_observer
{
public:
    Mock_observer( I_observable& observable )
    {
        observable.subscribe( m_handler );
    }

    MOCK_METHOD( void, notified, ( ) );

private:
    Observer_handler<Mock_observer> m_handler { *this, &Mock_observer::callback };

    void callback( void )
    {
        notified();
    }
};

#endif // MOCK_OBSERVER_H_
