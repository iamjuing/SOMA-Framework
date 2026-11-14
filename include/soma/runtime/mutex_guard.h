// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_MUTEX_GUARD_H_
#define SOMA_MUTEX_GUARD_H_

#include <cstdint>

#include "soma/runtime/mutex.h"
#include "soma/runtime/recursive_mutex.h"

namespace SOMA
{

    template<typename Mutex_t>
    class Basic_mutex_guard
    {
    public:
        explicit Basic_mutex_guard( Mutex_t& mutex );
        Basic_mutex_guard( Mutex_t& mutex,
                           uint32_t timeout_ms );
        ~Basic_mutex_guard();

        Basic_mutex_guard( const Basic_mutex_guard& ) = delete;
        Basic_mutex_guard& operator=( const Basic_mutex_guard& ) = delete;
        Basic_mutex_guard( Basic_mutex_guard&& ) = delete;
        Basic_mutex_guard& operator=( Basic_mutex_guard&& ) = delete;

        bool owns_lock() const;

        // Unlocks early. Safe to call more than once.
        void release();

    private:
        Mutex_t* mp_mutex;
        bool m_owns_lock;
    };

    template<typename Mutex_t>
    Basic_mutex_guard<Mutex_t>::Basic_mutex_guard( Mutex_t& mutex )
        : mp_mutex( &mutex ),
          m_owns_lock( true )
    {
        mp_mutex->lock();
    }

    template<typename Mutex_t>
    Basic_mutex_guard<Mutex_t>::Basic_mutex_guard( Mutex_t& mutex,
                                                   uint32_t timeout_ms )
        : mp_mutex( &mutex ),
          m_owns_lock( mutex.lock( timeout_ms ) )
    {
        // intentionally empty
    }

    template<typename Mutex_t>
    Basic_mutex_guard<Mutex_t>::~Basic_mutex_guard()
    {
        release();
    }

    template<typename Mutex_t>
    bool Basic_mutex_guard<Mutex_t>::owns_lock() const
    {
        return m_owns_lock;
    }

    template<typename Mutex_t>
    void Basic_mutex_guard<Mutex_t>::release()
    {
        if( m_owns_lock )
        {
            mp_mutex->unlock();
            m_owns_lock = false;
        }
    }

    using Mutex_guard = Basic_mutex_guard<Mutex>;
    using Recursive_mutex_guard = Basic_mutex_guard<Recursive_mutex>;
}

#endif // SOMA_MUTEX_GUARD_H_
