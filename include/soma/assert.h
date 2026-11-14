// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_ASSERT_H_
#define SOMA_ASSERT_H_

#include "soma/config.h"
#include "soma/internal/compiler_attributes.h"

#ifndef SOMA_CONFIG_PANIC_HANDLER
#error "soma_config.h must define SOMA_CONFIG_PANIC_HANDLER(file_name, line, message)."
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// Forwards the failure to the platform hook as String_view, int, String_view.
// The hook owns interrupt masking, diagnostics and termination. No RTOS or
// logging APIs are called here; a returning hook is followed by an infinite loop.
#if defined( __GNUC__ )
__attribute__( ( noreturn ) )
#endif
void SOMA_panic( const char* p_file_name,
                 int line,
                 const char* p_message );

#ifdef __cplusplus
}
#endif // __cplusplus

// Omit diagnostic strings at the call site in optimized builds. Debug source
// locations use only the file name, including with the portable fallback.
#ifdef NDEBUG
#define SOMA_PANIC( message ) SOMA_panic( 0, 0, 0 )
#else
#define SOMA_PANIC( message ) SOMA_panic( SOMA_COMPILER_FILE_NAME, __LINE__, ( message ) )
#endif

// Available to both C and C++ callers. Assertions remain active with NDEBUG;
// only diagnostic arguments are omitted by SOMA_PANIC.
#define SOMA_ASSERT( expression )                                         \
        do                                                                \
        {                                                                 \
            if( !( expression ) )                                         \
            {                                                             \
                SOMA_PANIC( #expression );                                 \
            }                                                             \
        } while( 0 )

// Preserve the existing SOMA assertion spelling for C++ clients.
#ifdef __cplusplus
#define ASSERT( expression ) SOMA_ASSERT( expression )
#endif

#endif // SOMA_ASSERT_H_
