// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cstdio>
#include <cstdlib>

#include "platform/windows/windows_host.h"
#include "src/main.h"

int main( void )
{
    if( initialize_windows_host() == false )
    {
        std::fputs( "Could not start the Windows console output worker.\n", stderr );

        return EXIT_FAILURE;
    }

    main_cpp();
}
