// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_MOCK_VARIABLE_OBSERVER_H_
#define SOMA_MOCK_VARIABLE_OBSERVER_H_

#include <gmock/gmock.h>

#include "soma/variables/variable.h"
#include "soma/variables/variable_observer_handler.h"

template<typename T>
class Mock_variable_observer
{
public:
    Mock_variable_observer( const Variable<T>& P )
        : m_P( P ),
          m_P_handler( P, *this, &Mock_variable_observer::callback )
    {
        // intentionally empty
    }

    MOCK_METHOD( void, changed, ( const T value ) );

private:
    void callback( void )
    {
        changed( m_P );
    }

    const Variable<T>& m_P;
    Variable_observer_handler<Mock_variable_observer> m_P_handler;
};

#endif // MOCK_VARIABLE_OBSERVER_H_
