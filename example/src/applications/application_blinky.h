// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef APPLICATION_BLINKY_H_
#define APPLICATION_BLINKY_H_

#include "src/active_components/blinky.h"
#include "src/applications/application_common.h"

class Application_blinky: public Application_common
{
public:
    Application_blinky( void )
        : Application_common( "Blinky" )
    {
        // intentionally empty
    }

private:
    Blinky m_blinky { PAL::m_pal_task, PAL::m_LED };
};

#endif // APPLICATION_BLINKY_H_
