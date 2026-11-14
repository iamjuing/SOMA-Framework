// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_EVENT_TASK_H_
#define SOMA_EVENT_TASK_H_

#include "soma/runtime/task.h"
#include "soma/runtime/b_task_timer.h"
#include "soma/runtime/b_task_event.h"

namespace SOMA
{
    class Event_task: public Task, public I_observable
    {
    public:
        Event_task( String_view name,
                    Task::Priority_e priority,
                    B_task_memory& memory );

        void register_timer( B_task_timer& timer );
        uint32_t register_event( B_task_event& event );

    private:
        Linked_list<B_task_timer> m_timer_list;
        Linked_list<B_task_event> m_event_list;

        B_task_timer* mp_earliest_timer = nullptr;

        B_task_timer* get_earliest_timer( void );

        void handle_events( uint32_t events_bit_mask );

        void loop( void ) override;
    };
} // namespace SOMA

#endif // EVENT_TASK_H_
