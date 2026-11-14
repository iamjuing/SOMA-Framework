// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_OBSERVER_H_
#define SOMA_OBSERVER_H_

#include "soma/containers/linked_list.h"

namespace SOMA
{

    ///
    /// @brief Abstract base class representing a data/event observer.
    ///        - the notifier shall call the notify() method whenever required (e.g. on data change)
    ///        - can be inserted in a single linked List
    class Observer
    {
    public:
        void notify( void );

    private:
        STATIC_LINKED_LIST_ITEM( Observer );

        virtual void notify_handler( void ) = 0;
    };

    ///
    /// @brief Called by the data owner to notify the observer, or by other the observer subscribed to the same data owner.
    /// @param value new value of the observed object
    inline void Observer::notify( void )
    {
        notify_handler();
    }
} // namespace SOMA

#endif // OBSERVER_H_
