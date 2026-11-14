// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/runtime/event_task.h"
#include "soma/runtime/b_task_event.h"
#include "soma/observer.h"

namespace SOMA
{
    B_task_event::B_task_event( Event_task& event_task,
                                const I_observable& observed_object,
                                Observer& observer )
        : m_event_task( event_task ),
          m_observed_object( observed_object )
    {
        m_notification_value = event_task.register_event( *this );
        m_observed_object.subscribe( m_observer_handler );
        subscribe( observer );
    }

    B_task_event::~B_task_event( void )
    {
        m_observed_object.unsubscribe( m_observer_handler );
    }

    void B_task_event::callback_observed_object( void )
    {
        m_event_task.notify_task_set_bits( m_notification_value );
    }

    void B_task_event::handle( void )
    {
        notify();
    }
} // namespace SOMA
