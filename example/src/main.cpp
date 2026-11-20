// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "src/applications/application_blinky.h"
#include "src/main.h"

#include "soma.h"

namespace
{
    constexpr size_t c_startup_memory_size_B = 2048u;
    SOMA::Task_memory<c_startup_memory_size_B> g_startup_memory;

    SOMA::Application_factory<Application_blinky> g_application_factory;

    void startup( void* )
    {
        auto& application = g_application_factory.create<Application_blinky>();

        ILOG << "SOMA Framework Example";
        ILOG << "Created application: " << application.name();

        application.launch();
    }
}

extern "C" [[noreturn]] void main_cpp( void )
{
    SOMA::run( startup, g_startup_memory );
}
