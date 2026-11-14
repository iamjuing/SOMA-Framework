// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/runtime/task.h"
#include "soma/runtime/scheduler_suspension_guard.h"
#include "../internal.h"

#include "soma/diagnostics/log.h"
#include "soma/runtime/task_memory.h"

// Supplied by the selected FreeRTOS port (or SOMA's host test port).
BaseType_t xPortIsInsideInterrupt( void );

namespace SOMA
{
    static_assert( sizeof( TickType_t ) * 8u == SOMA_CONFIG_TICK_TYPE_WIDTH_BITS,
                   "SOMA_CONFIG_TICK_TYPE_WIDTH_BITS must match the FreeRTOS tick type selected by the port." );

    namespace Internal
    {
        bool wait_task_notification_ticks( uint32_t& out_notification_value,
                                           uint32_t clear_on_entry_mask,
                                           uint32_t clear_on_exit_mask,
                                           TickType_t timeout_ticks )
        {
            bool rv_timed_out = true;

            const BaseType_t received = xTaskNotifyWait( clear_on_entry_mask,
                                                         clear_on_exit_mask,
                                                         &out_notification_value,
                                                         timeout_ticks );

            if( received == pdTRUE )
            {
                rv_timed_out = false;
            }

            return rv_timed_out;
        }
    } // namespace Internal

    ///
    /// @brief Ctor for static stack allocation.
    /// @param name              task name
    /// @param priority          FreeRTOS priority of the underlying task
    /// @param memory            statically allocated task memory (stack, TCB)
    Task::Task( String_view name,
                Priority_e priority,
                B_task_memory& memory )
        : m_name( name ),
          m_priority( priority ),
          m_memory( memory )
    {
        // intentionally empty
    }

    ///
    /// @brief Dtor.
    Task::~Task( void )
    {
        stop();
    }

    bool Task::is_active( const Task& task )
    {
        ASSERT( in_ISR() == false );

        bool rv_is_active = false;

        if( task.is_started() )
        {
            const void* native_handle = task.mp_task_handle.load();

            if( ( native_handle != nullptr ) &&
                ( native_handle == xTaskGetCurrentTaskHandle() ) )
            {
                rv_is_active = true;
            }
        }

        return rv_is_active;
    }

    ///
    /// @brief Creates and starts the task.
    /// @param priority          FreeRTOS priority of the underlying task
    /// @param stack_size        in words (not bytes!)
    void Task::start( void )
    {
        create_task();

        ILOG << "[SOMA] " << Color::green << "Starting task: '" << m_name << "'" << Color::standard;

        resume_task();
    }

    ///
    /// @brief Creates the underlying task and leaves it suspended.
    void Task::create_task( void )
    {
        ASSERT( is_started() == false );

        Scheduler_suspension_guard scheduler_suspension;

        TaskHandle_t native_handle = xTaskCreateStatic( task,
                                                        m_name.buffer().array().storage(),
                                                        static_cast<uint32_t>( m_memory.stack().size() ),
                                                        reinterpret_cast<void*>( this ),
                                                        static_cast<UBaseType_t>( m_priority ),
                                                        m_memory.stack().storage(),
                                                        &m_memory.TCB() );

        mp_task_handle.store( static_cast<void*>( native_handle ) );

        m_is_started = true;

        vTaskSuspend( native_handle );
    }

    ///
    /// @brief Makes a prepared task ready to run.
    void Task::resume_task( void )
    {
        ASSERT( is_started() );
        vTaskResume( static_cast<TaskHandle_t>( mp_task_handle.load() ) );
    }

    ///
    /// @brief Waits for a task notification.
    /// @param out_notification_value    received notification value
    /// @param clear_on_entry_mask       see xTaskNotifyWait documentation for details
    /// @param clear_on_exit_mask        see xTaskNotifyWait documentation for details
    /// @param timeout_ms                time to wait for the notification
    /// @retval true, if a notification was received in time
    ///         false if timed out
    bool Task::wait_task_notification( uint32_t& out_notification_value,
                                       uint32_t clear_on_entry_mask,
                                       uint32_t clear_on_exit_mask,
                                       uint32_t timeout_ms )
    {
        const TickType_t timeout_ticks = Internal::milliseconds_to_ticks( timeout_ms );
        const bool rv_timed_out = Internal::wait_task_notification_ticks( out_notification_value,
                                                                          clear_on_entry_mask,
                                                                          clear_on_exit_mask,
                                                                          timeout_ticks );

        return rv_timed_out;
    }

    ///
    /// @brief Waits for any task notification.
    ///        This is a counterpart of notify_task( void ) and notify_task_set_bits( uint32_t ),
    ///        where the notification does not carry any useful notification, and is used only as a simple synchronization mechanism.
    /// @param timeout_ms    time to wait for the notification
    /// @retval true, if the notification was received in time
    ///         false if timed out
    bool Task::wait_task_notification( uint32_t timeout_ms )
    {
        uint32_t notification_value = 0;

        return wait_task_notification( notification_value,
                                       Internal::c_no_notification_bits,
                                       Internal::c_all_notification_bits,
                                       timeout_ms );
    }

    ///
    /// @brief Indefinitely waits for a task notification.
    /// @param out_notification_value    received notification value
    /// @param clear_on_entry_mask       see xTaskNotifyWait documentation for details
    /// @param clear_on_exit_mask        see xTaskNotifyWait documentation for details
    void Task::wait_task_notification( uint32_t& out_notification_value,
                                       uint32_t clear_on_entry_mask,
                                       uint32_t clear_on_exit_mask )
    {
        xTaskNotifyWait( clear_on_entry_mask,
                         clear_on_exit_mask,
                         &out_notification_value,
                         portMAX_DELAY );
    }

    ///
    /// @brief Indefinitely waits for any task notification.
    ///        This waits for the no-payload notification sent to this task.
    void Task::wait_task_notification( void )
    {
        xTaskNotifyWait( Internal::c_no_notification_bits,
                         Internal::c_all_notification_bits,
                         nullptr,
                         portMAX_DELAY );
    }

    ///
    /// @brief Sets notification bits on this task.
    /// @param value  bits to OR into the pending notification value
    /// @retval true if the notification was accepted
    /// @retval false if the task is stopped or the notification was not accepted
    bool Task::notify_task_set_bits( uint32_t value )
    {
        bool rv_notified = false;
        const TaskHandle_t native_handle = static_cast<TaskHandle_t>( mp_task_handle.load() );

        if( m_is_started && native_handle )
        {
            if( in_ISR() )
            {
                BaseType_t yield = pdFALSE;
                const BaseType_t result = xTaskNotifyFromISR( native_handle, value, eSetBits, &yield );

                portYIELD_FROM_ISR( yield );

                rv_notified = result == pdPASS;
            }
            else
            {
                rv_notified = xTaskNotify( native_handle, value, eSetBits ) == pdPASS;
            }
        }

        return rv_notified;
    }

    ///
    /// @brief Sends a no-payload notification to this task for synchronization.
    ///        This is a counterpart of wait_task_notification( void ).
    void Task::notify_task( void )
    {
        notify_task_set_bits( 0u );
    }

    ///
    /// @brief Stops and destroys the task. Every child class shall call this in its destructor.
    ///        NOTE: This method is not usually called in the production code.
    ///              It is required mainly by unit tests. If not used, the unit test framework will detect it
    ///              and fail immediately.
    void Task::stop( void )
    {
        if( m_is_started )
        {
            m_is_started = false;

            vTaskDelete( static_cast<TaskHandle_t>( mp_task_handle.load() ) );
        }
    }

    ///
    /// @brief Common (static) method passed to FreeRTOS as the task's entry point with pointer to the starting object,
    ///        that eventually launches the object specific loop.
    /// @param p_task_creator pointer to an object that created the FreeRTOS class via ::start() method
    void Task::task( void* p_task_creator )
    {
        ASSERT( p_task_creator != nullptr );

        auto* p_object = reinterpret_cast<Task*>( p_task_creator );

        p_object->m_last_wakeup_time_ticks = static_cast<Tick_t>( xTaskGetTickCount() );

        p_object->loop();

        ILOG << "[SOMA] " << Color::red << "Task ended: '" << p_object->m_name << "'" << Color::standard;

        vTaskDelete( nullptr );
    }

    void Task::sleep_period( uint32_t period_ms )
    {
        TickType_t last_wakeup_time = static_cast<TickType_t>( m_last_wakeup_time_ticks );

        vTaskDelayUntil( &last_wakeup_time, Internal::milliseconds_to_ticks( period_ms ) );

        m_last_wakeup_time_ticks = static_cast<Tick_t>( last_wakeup_time );
    }

    void Task::sleep( uint32_t time_ms )
    {
        TickType_t native_time_ticks = Internal::milliseconds_to_ticks( time_ms );

        // Add one more tick to ensure the timeout is not less than the requested time.
        // NOTE: This ensures that the timeout is in range of <X, X + 1> as the FreeRTOS time blocking API
        //       precission is in range <X-1, X> where X is number of ticks to block.
        ++native_time_ticks;

        vTaskDelay( native_time_ticks );
    }

    bool Task::in_ISR( void )
    {
        return ::xPortIsInsideInterrupt() == pdTRUE;
    }
} // namespace SOMA
