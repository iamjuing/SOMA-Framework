// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TASK_H_
#define SOMA_TASK_H_

#include <atomic>
#include <stdio.h>
#include <limits>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "soma/i_object_enumerator.h"
#include "soma/assert.h"
#include "soma/internal/port_traits.h"
#include "soma/time.h"
#include "soma/text/string_view.h"

namespace SOMA
{
    template<typename Platform_t>
    class B_application;

    static_assert( SOMA_CONFIG_SYNC_WAIT_CHUNK_MS > 0u &&
                   SOMA_CONFIG_SYNC_WAIT_CHUNK_MS <= std::numeric_limits<uint32_t>::max(),
                   "SOMA_CONFIG_SYNC_WAIT_CHUNK_MS must be a positive uint32_t millisecond interval." );
    static_assert( ( static_cast<uint64_t>( SOMA_CONFIG_SYNC_WAIT_CHUNK_MS ) * Internal::c_tick_rate_hz ) / 1000u > 0u &&
                   ( static_cast<uint64_t>( SOMA_CONFIG_SYNC_WAIT_CHUNK_MS ) * Internal::c_tick_rate_hz ) / 1000u < portMAX_DELAY,
                   "SOMA_CONFIG_SYNC_WAIT_CHUNK_MS must convert to at least one tick and less than portMAX_DELAY." );

    class B_task_memory;

    ///
    /// @brief Base class representing a FreeRTOS task.
    class Task: public I_object_enumerator<Task>
    {
    public:
    #if configMAX_PRIORITIES == 7 // mostly used default
        enum class Priority_e
        {
            idle         = 0,
            low          = 1,
            below_normal = 2,
            normal       = 3,
            above_normal = 4,
            high         = 5,
            realtime     = 6
        };

    #else

        class Priority_e
        {
        public:
            constexpr Priority_e( unsigned int priority )
                : m_priority( priority )
            {
                ASSERT( priority < configMAX_PRIORITIES );
            }

            // only these priorities are clear
            static constexpr UBaseType_t idle = 0;
            static constexpr UBaseType_t realtime = configMAX_PRIORITIES - 1;

            constexpr operator UBaseType_t() const
            {
                return m_priority;
            }

        private:
            UBaseType_t m_priority;
        };

    #endif

        Task( String_view name,
              Priority_e priority,
              B_task_memory& storage );

        virtual ~Task( void );

        bool is_started( void ) const;

        void start( void );
        void stop( void );

        virtual void loop( void ) = 0;

        const B_task_memory& memory( void ) const;
        String_view name( void ) const;

        static bool wait_task_notification( uint32_t& out_notification_value,
                                            uint32_t clear_on_entry_mask,
                                            uint32_t clear_on_exit_mask,
                                            uint32_t timeout_ms );
        static bool wait_task_notification( uint32_t timeout_ms );
        static void wait_task_notification( uint32_t& out_notification_value,
                                            uint32_t clear_on_entry_mask,
                                            uint32_t clear_on_exit_mask );
        static void wait_task_notification( void );

        ///
        /// @brief Sends a notification to this task.
        /// @return true if accepted; false if the task is stopped or the action could not be applied.
        bool notify_task_set_bits( uint32_t value );

        void notify_task( void );

        static void sleep( uint32_t time_ms );
        void sleep_period( uint32_t period_ms );

        static bool in_ISR( void );

        static bool is_active( const Task& task );

    private:
        template<typename Platform_t>
        friend class B_application;

        static constexpr uint32_t mc_minimum_priority_cnt = 7;

        static_assert( configMAX_PRIORITIES >= mc_minimum_priority_cnt, "Not enough task priority levels!" );

        using Tick_t = Internal::Port_tick_t;

        String_view m_name;
        std::atomic<void*> mp_task_handle { nullptr };
        Priority_e m_priority;
        B_task_memory& m_memory;

        std::atomic_bool m_is_started = false;

        Tick_t m_last_wakeup_time_ticks;

        void create_task( void );
        void resume_task( void );

        static void task( void* p_task_creator );
    };

    ///
    /// @brief Retrieves whether the task is running.
    /// @retval true, if the task is running
    ///         false otherwise
    inline bool Task::is_started( void ) const
    {
        return m_is_started;
    }

    ///
    /// @brief Blocks the task for AT LEAST the specified number of milliseconds.
    /// @param sleep_ms number of milliseconds to sleep
    inline const B_task_memory& Task::memory( void ) const
    {
        return m_memory;
    }

    ///
    /// @brief Retrieves the task's name.
    /// @retval task's name
    inline String_view Task::name( void ) const
    {
        return m_name;
    }

    ///
    /// @brief Checks whether the code calling this method runs in an ISR context.
    /// @retval true, if the code calling this method runs in an ISR context
    ///         false othewise
} // namespace SOMA

#endif // TASK_H_
