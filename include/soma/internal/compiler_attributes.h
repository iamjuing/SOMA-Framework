// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_INTERNAL_COMPILER_ATTRIBUTES_H_
#define SOMA_INTERNAL_COMPILER_ATTRIBUTES_H_

// Prefer a file name only literal. The portable fallback returns the basename
// too, but the compiler may retain the full __FILE__ literal in the image.
#if defined( __FILE_NAME__ )
    #define SOMA_COMPILER_FILE_NAME __FILE_NAME__
#else
    #ifdef __cplusplus
    static constexpr const char* SOMA_compiler_file_name( const char* p_path )
    #else
    static inline const char* SOMA_compiler_file_name( const char* p_path )
    #endif
    {
        const char* p_file_name = p_path;

        for( ; *p_path != '\0'; ++p_path )
        {
            if( ( *p_path == '/' ) || ( *p_path == '\\' ) )
            {
                p_file_name = p_path + 1;
            }
        }

        return p_file_name;
    }

    #define SOMA_COMPILER_FILE_NAME SOMA_compiler_file_name( __FILE__ )
#endif

// C compatibility
#if defined( __clang__ ) || defined( __GNUC__ )
    #define SOMA_COMPILER_PRINTF_FORMAT( format_index, first_argument_index ) \
            __attribute__ ( ( format( printf, format_index, first_argument_index ) ) )
#else
    #define SOMA_COMPILER_PRINTF_FORMAT( format_index, first_argument_index )
#endif

#endif // SOMA_INTERNAL_COMPILER_ATTRIBUTES_H_
