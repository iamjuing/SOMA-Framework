// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_OBSERVABLE_H_
#define SOMA_I_OBSERVABLE_H_

#include "soma/observer.h"

namespace SOMA
{
    class I_observable
    {
    public:
        void subscribe( Observer& observer ) const
        {
            m_observer_list.insert( observer );
        }

        void unsubscribe( Observer& observer ) const
        {
            m_observer_list.remove( observer );
        }

    protected:
        void notify( void )
        {
            for( Observer& observer: m_observer_list )
            {
                observer.notify();
            }
        }

    private:
        mutable Linked_list<Observer> m_observer_list;
    };
} // namespace SOMA

#endif // I_OBSERVABLE_H_
