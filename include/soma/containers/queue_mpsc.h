// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_QUEUE_MPSC_H_
#define SOMA_QUEUE_MPSC_H_

#include "soma/containers/queue_spsc.h"
#include "soma/runtime/critical_section.h"
#include "soma/runtime/mutex_guard.h"

namespace SOMA
{
    ///
    /// @brief Multiple-producer, single-consumer queue with external or owned storage.
    ///
    /// Queue_MPSC<T> uses storage supplied by the caller. Queue_MPSC<T, N>
    /// owns N slots and derives from Queue_MPSC<T>. Capacity must be positive;
    /// external storage must outlive the queue.
    ///
    /// @code
    /// #include <cstdint>
    /// using SOMA::Array;
    /// using SOMA::Queue_MPSC;
    /// void queue_mpsc_examples()
    /// {
    ///     uint8_t external[4]{};
    ///     Queue_MPSC<uint8_t> view { "external", Array<uint8_t>{ external } };
    ///     Queue_MPSC<uint8_t, 4> owned { "owned" };
    ///     Queue_MPSC<uint8_t>& common = owned;
    ///
    ///     view.push( 1 );
    ///     owned.push( 2 );
    ///     uint8_t item{};
    ///     common.pop( item );
    /// }
    /// @endcode
    template<typename T, size_t N = dynamic_extent>
    class Queue_MPSC;

    template<typename T>
    class Queue_MPSC<T, dynamic_extent>: public I_single_consumer_queue<T>
    {
    public:
        explicit Queue_MPSC( String_view name,
                             Array<T> array );
        virtual ~Queue_MPSC() = default;

        size_t capacity( void ) const override;

        size_t size( void ) const override;
        size_t space( void ) const override;
        bool is_empty( void ) const override;
        bool is_full( void ) const override;

        bool push( const T& item ) override;
        size_t push( Buffer_view<T> data ) override;

        bool pop( T& out_item ) override;
        size_t pop( Buffer<T>& out_buffer ) override;

        // NOTE: The returned buffer does not necessarily contain all items available (because of queue wrap around).
        //       Need to call multiple times until empty buffer is returned.
        Buffer_view<T> view_items( void ) const override;
        void displace( size_t item_cnt ) override;

        Signal& signal_queue_empty( void ) override;
        Signal& signal_space_available( void ) override;
        Signal& signal_queue_full( void ) override;
        Signal& signal_items_available( void ) override;

    private:
        Queue_SPSC<T> m_queue;
        T* mp_raw_buffer; // required for ISR

        Mutex m_mutex;
        std::atomic_size_t m_future_items_cnt = 0u;

        size_t push_from_ISR( Buffer_view<T> data );
        size_t push_from_task( Buffer_view<T> data );
    };

    template<typename T>
    Queue_MPSC<T, dynamic_extent>::Queue_MPSC( String_view name,
                                               Array<T> array )
        : m_queue( name, array ),
          mp_raw_buffer( array.storage() )
    {
        // intentionally empty
    }

    template<typename T>
    Signal& Queue_MPSC<T, dynamic_extent>::signal_space_available( void )
    {
        return m_queue.signal_space_available();
    }

    template<typename T>
    Signal& Queue_MPSC<T, dynamic_extent>::signal_queue_full( void )
    {
        return m_queue.signal_queue_full();
    }

    template<typename T>
    Signal& Queue_MPSC<T, dynamic_extent>::signal_items_available( void )
    {
        return m_queue.signal_items_available();
    }

    template<typename T>
    Signal& Queue_MPSC<T, dynamic_extent>::signal_queue_empty( void )
    {
        return m_queue.signal_queue_empty();
    }

    template<typename T>
    bool Queue_MPSC<T, dynamic_extent>::is_empty( void ) const
    {
        return m_queue.is_empty();
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::capacity( void ) const
    {
        return m_queue.capacity();
    }

    template<typename T>
    bool Queue_MPSC<T, dynamic_extent>::is_full( void ) const
    {
        return m_queue.is_full();
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::size( void ) const
    {
        return m_queue.size();
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::space( void ) const
    {
        return m_queue.space();
    }

    template<typename T>
    bool Queue_MPSC<T, dynamic_extent>::push( const T& item )
    {
        // NOTE: const_cast is safe Buffer_view has only const methods
        return ( push( Buffer_view( Array( const_cast<T*>( &item ), 1 ) ) ) == 1 );
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::push( Buffer_view<T> data )
    {
        size_t rv_items_pushed = 0;

        if( Task::in_ISR() )
        {
            rv_items_pushed = push_from_ISR( data );
        }
        else
        {
            rv_items_pushed = push_from_task( data );
        }

        return rv_items_pushed;
    }

    template<typename T>
    bool Queue_MPSC<T, dynamic_extent>::pop( T& out_item )
    {
        return m_queue.pop( out_item );
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::pop( Buffer<T>& out_buffer )
    {
        return m_queue.pop( out_buffer );
    }

    template<typename T>
    Buffer_view<T> Queue_MPSC<T, dynamic_extent>::view_items( void ) const
    {
        return m_queue.view_items();
    }

    template<typename T>
    void Queue_MPSC<T, dynamic_extent>::displace( size_t items_cnt )
    {
        m_queue.displace( items_cnt );
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::push_from_ISR( Buffer_view<T> data )
    {
        size_t rv_items_pushed = 0;

        if( data.size() > 0u )
        {
            // block other interrupts
            Critical_section_guard critical_section;

            if( m_mutex.is_locked() )
            // a task is in process of pushing data
            {
                size_t future_items_cnt = m_future_items_cnt.load( std::memory_order_relaxed );
                size_t to_copy = std::min( m_queue.space() - future_items_cnt, data.size() );

                if( to_copy > 0u )
                {
                    Buffer<T> target_buffer = m_queue.get_space();     // same buffer as seen by the task that locked the queue

                    if( future_items_cnt < target_buffer.space() )
                    {
                        // pretend the data is there
                        target_buffer.place( future_items_cnt );

                        rv_items_pushed = target_buffer.insert( data );

                        if( rv_items_pushed < to_copy )
                        // wrap around, second attempt needed
                        {
                            // buffer for the second attempt
                            target_buffer = Buffer( Array( mp_raw_buffer,
                                                           to_copy - rv_items_pushed ) );
                            rv_items_pushed += target_buffer.insert( data.sub_buffer( rv_items_pushed ) );
                        }
                    }
                    else
                    // reserved by the task until the end of the buffer
                    {
                        target_buffer = Buffer( Array( &mp_raw_buffer[future_items_cnt - target_buffer.space()],
                                                       to_copy ) );

                        rv_items_pushed = target_buffer.insert( data.sub_buffer( rv_items_pushed ) );
                    }

                    m_future_items_cnt.fetch_add( rv_items_pushed, std::memory_order_relaxed );

                    // NOTE: The task context notifies the observers.
                }
            }
            else
            // free to push data directly
            {
                rv_items_pushed = m_queue.push( data );
            }
        }

        return rv_items_pushed;
    }

    template<typename T>
    size_t Queue_MPSC<T, dynamic_extent>::push_from_task( Buffer_view<T> data )
    {
        size_t rv_items_pushed = 0;

        if( data.size() > 0u )
        {
            size_t to_copy = 0u;

            // block out other writers before entering the critical section
            Mutex_guard mutex_guard( m_mutex );

            {
                Critical_section_guard critical_section;

                // place items that an ISR could have pushed between the lock and entering the critical section
                if( m_future_items_cnt.load( std::memory_order_relaxed ) > 0u )
                {
                    m_queue.place( m_future_items_cnt.load( std::memory_order_relaxed ) );
                }

                to_copy = std::min( m_queue.space(), data.size() );

                // reserve the beginning part of the free space
                m_future_items_cnt.store( to_copy, std::memory_order_relaxed );
            }

            // interrupts enabled while data is copied

            Buffer<T> target_buffer = m_queue.get_space();

            if( to_copy > 0u )
            {
                rv_items_pushed = target_buffer.insert( data );

                if( rv_items_pushed < to_copy )
                // wrap around, second attempt needed
                {
                    // buffer for the second attempt
                    target_buffer = Buffer( Array( mp_raw_buffer,
                                                   to_copy - rv_items_pushed ) );
                    rv_items_pushed += target_buffer.insert( data.sub_buffer( rv_items_pushed ) );
                }
            }

            {
                Critical_section_guard critical_section;

                // place all items copied in both, the task and ISR calls
                rv_items_pushed = m_future_items_cnt.load( std::memory_order_relaxed );
                m_queue.place( rv_items_pushed );

                m_future_items_cnt.store( 0u, std::memory_order_relaxed );

                // WARNING: Unlock the mutex first to avoid ISR to write additional data that would not be placed in the queue.
                mutex_guard.release();
            }
        }

        return rv_items_pushed;
    }

    template<typename T, size_t N>
    class Queue_MPSC: public Queue_MPSC<T, dynamic_extent>
    {
    public:
        static_assert( N > 0u, "Queue capacity must be positive" );

        explicit Queue_MPSC( String_view name = "unnamed" );

    private:
        T m_storage[N];
    };


    template<typename T, size_t N>
    Queue_MPSC<T, N>::Queue_MPSC( String_view name )
        : Queue_MPSC<T, dynamic_extent>( name, Array( m_storage, N ) )
    {
        // intentionally empty
    }
} // namespace SOMA

#endif // QUEUE_MPSC_H_
