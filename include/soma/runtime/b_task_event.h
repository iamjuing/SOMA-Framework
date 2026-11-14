// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_B_TASK_EVENT_H_
#define SOMA_B_TASK_EVENT_H_

#include <cstdint>

#include "soma/i_observable.h"
#include "soma/containers/linked_list.h"
#include "soma/observer_handler.h"

namespace SOMA
{
    class Event_task;
    class Observer;

    ///
    /// @brief Base class for event handling in Event_task context.
    ///        Bridges an observable object's notifications to task context notification.
    ///
    ///        NOTE: Consider using Task_event<T> template for simpler usage with automatic Observer management
    class B_task_event: public I_observable
    {
    public:
        B_task_event( Event_task& event_task,
                      const I_observable& observed_object,
                      Observer& handler );
        ~B_task_event( void );

        uint32_t get_notification_value( void ) const;

        void handle( void );

    private:
        STATIC_LINKED_LIST_ITEM( B_task_event );

        Event_task& m_event_task;
        const I_observable& m_observed_object;

        uint32_t m_notification_value = 0u;
        Observer_handler<B_task_event> m_observer_handler { *this, &B_task_event::callback_observed_object };

        void callback_observed_object( void );
    };

    inline uint32_t B_task_event::get_notification_value( void ) const
    {
        return m_notification_value;
    }
} // namespace SOMA

#endif // B_TASK_EVENT_H_
