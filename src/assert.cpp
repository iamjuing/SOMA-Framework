// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "soma/assert.h"
#include "soma/text/string_view.h"

extern "C" void SOMA_panic( const char* p_file_name,
                            int line,
                            const char* p_message )
{
    const SOMA::String_view file_name = p_file_name != nullptr ? SOMA::String_view( p_file_name ) : SOMA::String_view {};
    const SOMA::String_view message = p_message != nullptr ? SOMA::String_view( p_message ) : SOMA::String_view {};

    SOMA_CONFIG_PANIC_HANDLER( file_name, line, message );

    // The platform hook owns shutdown. Never return if it unexpectedly does.
    for( ;; )
    {
    }
}
