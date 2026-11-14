// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_QUEUE_SPSC_H_
#define SOMA_QUEUE_SPSC_H_

#include <cstddef>

#include "soma/containers/ring_buffer.h"
#include "soma/containers/buffer.h"
#include "soma/containers/i_single_producer_queue.h"
#include "soma/containers/i_single_consumer_queue.h"
#include "soma/runtime/signal.h"
#include "soma/containers/i_queue_profile.h"

namespace SOMA
{
    ///
    /// @brief Single-producer, single-consumer queue with external or owned storage.
    ///
    /// Queue_SPSC<T> uses storage supplied by the caller. Queue_SPSC<T, N>
    /// owns N slots and derives from Queue_SPSC<T>. Capacity must be positive;
    /// external storage must outlive the queue.
    ///
    /// @code
    /// #include <cstdint>
    /// using SOMA::Array;
    /// using SOMA::Queue_SPSC;
    /// void queue_spsc_examples()
    /// {
    ///     uint8_t external[4]{};
    ///     Queue_SPSC<uint8_t> view { "external", Array<uint8_t>{ external } };
    ///     Queue_SPSC<uint8_t, 4> owned { "owned" };
    ///     Queue_SPSC<uint8_t>& common = owned;
    ///
    ///     view.push( 1 );
    ///     owned.push( 2 );
    ///     uint8_t item{};
    ///     common.pop( item );
    /// }
    /// @endcode
    template<typename T, size_t N = dynamic_extent>
    class Queue_SPSC;

    template<typename T>
    class Queue_SPSC<T, dynamic_extent>: public I_queue_profile, public I_single_producer_queue<T>,
        public I_single_consumer_queue<T>
    {
    public:
        explicit Queue_SPSC( String_view name,
                             Array<T> array );
        ~Queue_SPSC() override = default;

        void clear( void );

        size_t item_size_B( void ) const override;
        size_t capacity( void ) const override;
        bool is_empty( void ) const override;
        bool is_full( void ) const override;

        size_t size( void ) const override;
        size_t space( void ) const override;

        bool push( const T& item ) override;
        size_t push( Buffer_view<T> data ) override;

        // NOTE: Not the entire free space buffer is necessarily returned (because of queue wrap around).
        //       Need to call multiple times.
        Buffer<T> get_space( void ) const override;
        void place( size_t items_cnt ) override;

        bool pop( T& out_item ) override;
        size_t pop( Buffer<T>& out_buffer ) override;

        // NOTE: The returned buffer does not necessarily contain all items available (because of queue wrap around).
        //       Need to call multiple times until empty buffer is returned.
        Buffer_view<T> view_items( void ) const override;
        void displace( size_t items_cnt ) override;

        Signal& signal_queue_empty( void ) override;
        Signal& signal_space_available( void ) override;
        Signal& signal_queue_full( void ) override;
        Signal& signal_items_available( void ) override;

    private:
        Ring_buffer<T> m_storage;

        Signal m_signal_queue_full;
        Signal m_signal_queue_empty;
        Signal m_signal_items_available;
        Signal m_signal_space_available;
    };

    template<typename T>
    Queue_SPSC<T, dynamic_extent>::Queue_SPSC( String_view name,
                                               Array<T> array )
        : I_queue_profile( name ),
          m_storage( array )
    {
        ASSERT( array.size() > 0u );
    }

    template<typename T>
    Buffer<T> Queue_SPSC<T, dynamic_extent>::get_space( void ) const
    {
        return m_storage.get_space();
    }

    template<typename T>
    Buffer_view<T> Queue_SPSC<T, dynamic_extent>::view_items( void ) const
    {
        return m_storage.view_items();
    }

    template<typename T>
    void Queue_SPSC<T, dynamic_extent>::place( size_t items_cnt )
    {
        if( items_cnt > 0u )
        {
            size_t current_item_cnt = m_storage.size();

            m_storage.place( items_cnt );

            m_peak_items_cnt = std::max( m_peak_items_cnt, current_item_cnt + items_cnt );

            if( m_storage.size() <= items_cnt )
            {
                m_signal_items_available.emit();
            }

            if( m_storage.space() == 0u )
            {
                m_signal_queue_full.emit();
            }
        }
    }

    template<typename T>
    bool Queue_SPSC<T, dynamic_extent>::push( const T& item )
    {
        size_t current_item_cnt = m_storage.size();

        bool rv_item_pushed = m_storage.insert( item );

        if( m_storage.size() <= 1 )
        {
            m_signal_items_available.emit();
        }

        if( rv_item_pushed )
        {
            m_peak_items_cnt = std::max( m_peak_items_cnt, current_item_cnt + 1 );
        }
        else
        {
            m_signal_queue_full.emit();
        }

        return rv_item_pushed;
    }

    template<typename T>
    size_t Queue_SPSC<T, dynamic_extent>::push( Buffer_view<T> data )
    {
        size_t current_item_cnt = m_storage.size();
        size_t rv_items_pushed = 0u;

        if( data.size() > 0u )
        {
            rv_items_pushed = m_storage.insert( data );

            m_peak_items_cnt = std::max( m_peak_items_cnt, current_item_cnt + rv_items_pushed );

            if( m_storage.size() <= rv_items_pushed )
            {
                m_signal_items_available.emit();
            }

            if( rv_items_pushed < data.size() )
            {
                m_signal_queue_full.emit();
            }
        }

        return rv_items_pushed;
    }

    template<typename T>
    void Queue_SPSC<T, dynamic_extent>::displace( size_t items_cnt )
    {
        if( items_cnt > 0u )
        {
            m_storage.displace( items_cnt );

            if( m_storage.space() <= items_cnt )
            {
                m_signal_space_available.emit();
            }

            if( m_storage.size() == 0u )
            {
                m_signal_queue_empty.emit();
            }
        }
    }

    template<typename T>
    bool Queue_SPSC<T, dynamic_extent>::pop( T& out_item )
    {
        bool rv_item_popped = m_storage.remove( out_item );

        if( m_storage.space() <= 1 )
        {
            m_signal_space_available.emit();
        }

        if( rv_item_popped == false )
        {
            m_signal_queue_empty.emit();
        }

        return rv_item_popped;
    }

    template<typename T>
    size_t Queue_SPSC<T, dynamic_extent>::pop( Buffer<T>& out_buffer )
    {
        size_t rv_items_removed = 0u;

        if( out_buffer.space() > 0u )
        {
            rv_items_removed = m_storage.remove( out_buffer );

            if( m_storage.space() <= rv_items_removed )
            {
                m_signal_space_available.emit();
            }

            if( rv_items_removed == 0u )
            {
                m_signal_queue_empty.emit();
            }
        }

        return rv_items_removed;
    }

    template<typename T>
    size_t Queue_SPSC<T, dynamic_extent>::size( void ) const
    {
        return m_storage.size();
    }

    template<typename T>
    size_t Queue_SPSC<T, dynamic_extent>::space( void ) const
    {
        return m_storage.space();
    }

    template<typename T>
    void Queue_SPSC<T, dynamic_extent>::clear( void )
    {
        displace( m_storage.size() );
    }

    template<typename T>
    bool Queue_SPSC<T, dynamic_extent>::is_empty( void ) const
    {
        return m_storage.is_empty();
    }

    template<typename T>
    bool Queue_SPSC<T, dynamic_extent>::is_full( void ) const
    {
        return m_storage.is_full();
    }

    template<typename T>
    Signal& Queue_SPSC<T, dynamic_extent>::signal_space_available( void )
    {
        return m_signal_space_available;
    }

    template<typename T>
    Signal& Queue_SPSC<T, dynamic_extent>::signal_queue_full( void )
    {
        return m_signal_queue_full;
    }

    template<typename T>
    Signal& Queue_SPSC<T, dynamic_extent>::signal_items_available( void )
    {
        return m_signal_items_available;
    }

    template<typename T>
    Signal& Queue_SPSC<T, dynamic_extent>::signal_queue_empty( void )
    {
        return m_signal_queue_empty;
    }

    template<typename T>
    size_t Queue_SPSC<T, dynamic_extent>::capacity( void ) const
    {
        return m_storage.capacity();
    }

    template<typename T>
    size_t Queue_SPSC<T, dynamic_extent>::item_size_B( void ) const
    {
        return sizeof( T );
    }

    template<typename T, size_t N>
    class Queue_SPSC: public Queue_SPSC<T, dynamic_extent>
    {
    public:
        static_assert( N > 0u, "Queue capacity must be positive" );

        explicit Queue_SPSC( String_view name = "unnamed" );

    private:
        T m_storage[N];
    };

    template<typename T, size_t N>
    Queue_SPSC<T, N>::Queue_SPSC( String_view name )
        : Queue_SPSC<T, dynamic_extent>( name, Array( m_storage, N ) )
    {
        // intentionally empty
    }
} // namespace SOMA

#endif // QUEUE_SPSC_H_
