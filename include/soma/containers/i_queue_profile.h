// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_I_QUEUE_PROFILE_H
#define SOMA_I_QUEUE_PROFILE_H

#include "soma/i_object_enumerator.h"
#include "soma/text/string_view.h"

namespace SOMA
{

    class I_queue_profile: public I_object_enumerator<I_queue_profile>
    {
    public:
        explicit I_queue_profile( String_view name );
        virtual ~I_queue_profile() = default;

        String_view name( void ) const;
        size_t peak_cnt( void ) const;

        virtual size_t capacity( void ) const = 0;
        virtual size_t item_size_B( void ) const = 0;

    protected:
        size_t m_peak_items_cnt = 0u;

    private:
        String_view m_name;
    };

    inline I_queue_profile::I_queue_profile( String_view name )
        : m_name( name )
    {
        // intentionally empty
    }

    inline String_view I_queue_profile::name( void ) const
    {
        return m_name;
    }

    inline size_t I_queue_profile::peak_cnt( void ) const
    {
        return m_peak_items_cnt;
    }
} // namespace SOMA

#endif // I_QUEUE_PROFILE_H
