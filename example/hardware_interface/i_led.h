// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef HARDWARE_INTERFACE_I_LED_H_
#define HARDWARE_INTERFACE_I_LED_H_

class I_LED
{
public:
    virtual ~I_LED( void ) = default;
    virtual void set( bool is_on ) = 0;
};

#endif // HARDWARE_INTERFACE_I_LED_H_
