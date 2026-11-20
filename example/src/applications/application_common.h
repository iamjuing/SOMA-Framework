// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef APPLICATION_COMMON_H_
#define APPLICATION_COMMON_H_

#include <cstdint>

#include "platform/windows/pal.h"
#include "soma/array.h"
#include "soma/diagnostics/log_streamer.h"
#include "soma/runtime/b_application.h"
#include "soma/text/string_view.h"

class Application_common: public SOMA::B_application<PAL>
{
public:
    explicit Application_common( SOMA::String_view name )
        : SOMA::B_application<PAL>( name )
    {
        m_log.expose();
    }

    ~Application_common( void )
    {
        m_log.hide();
    }

private:
    static constexpr uint32_t mc_truncation_pause_ms = 20u;
    SOMA::Array<char, PAL::mc_log_capacity_B> m_log_storage;

    SOMA::Log m_log { m_log_storage };
    SOMA::Log_streamer m_log_streamer { PAL::m_pal_task,
                                        m_log,
                                        mc_truncation_pause_ms,
                                        PAL::m_console };
};

#endif // APPLICATION_COMMON_H_
