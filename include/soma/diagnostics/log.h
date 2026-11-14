// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_LOG_H_
#define SOMA_LOG_H_

#ifdef __cplusplus
#include <atomic>
#include <cstddef>
#include <cstdint>
#endif
#include <stdarg.h>

#include "soma/config.h"

#include "soma/internal/compiler_attributes.h"

#ifdef __cplusplus
#include "soma/array.h"
#include "soma/runtime/signal.h"
#include "soma/containers/i_single_consumer_queue.h"
#include "soma/observer_handler.h"
#include "soma/containers/queue_mpsc.h"
#include "soma/text/text_output.h"

namespace SOMA
{
#ifndef SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH
#error "soma_config.h must define SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH."
#endif

    class Log_streamer;

    ///
    /// @brief Passive, system-wide log storage.
    ///        - formats log lines and stores them until a Log_streamer consumes them
    ///        - thread safe for multiple producers
    ///        - supports logging from ISR
    ///          WARNING: Only ISRs that are free to call FreeRTOS ISR API can log. Otherwise the behavior is undefined.
    ///                   See configMAX_SYSCALL_INTERRUPT_PRIORITY FreeRTOS configuration.
    class Log
    {
    public:
        enum class Log_severity_e
        {
            debug,
            info,
            error
        };

        ///
        /// @brief RAII wrapper for stream-based logging.
        ///        Composes a line on the caller's stack and stores it on destruction.
        class Text_log_output: public Text_output
        {
        public:
            Text_log_output( Log& log,
                             Log_severity_e severity,
                             const char* p_file_name,
                             int line,
                             Array<char> line_buffer );
            ~Text_log_output();

            Text_log_output( const Text_log_output& ) = delete;
            Text_log_output& operator=( const Text_log_output& ) = delete;
            Text_log_output( Text_log_output&& ) = delete;
            Text_log_output& operator=( Text_log_output&& ) = delete;

            void vformat( const char* p_format,
                          va_list args );

        private:

            class Stream: public I_blocking_output_stream
            {
            public:
                explicit Stream( String& output_text );
                ~Stream() override;

                bool write( Buffer_view<uint8_t> data ) override;

            private:
                String& m_output_text;
            };

            Log& m_log;
            String m_line_str;
            Stream m_stream;
            Log_severity_e m_severity;
        };

        explicit Log( Array<char> storage );
        ~Log( void );

        void expose( void );
        void hide( void );
        static Log* get_exposed( void );

        // exposes an ordered view of the log content as an array of character arrays
        // WARNING: Only valid when the system is paused or stopped.
        // NOTE: Useful for log flush in a panic state.
        Array<Array<const char>, 2> raw_content( void ) const;

        size_t capacity( void ) const;
        static constexpr size_t line_buffer_size( void );

        void log( const char* p_format,
                  va_list args );

    private:
        friend class Log_streamer;
        friend class Text_log_output;

        static inline Log* mp_log = nullptr;

        static constexpr String_view mc_uptime_separator { ": " };
        static constexpr String_view mc_str_line_trunc { " <...>" };
        static constexpr String_view mc_log_truncation { "[SOMA] <LOG TRUNCATED>" };
        static constexpr size_t mc_uptime_digits = 10u;

        static_assert( SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH > mc_str_line_trunc.length(),
                       "SOMA log message limit is too small." );

        static constexpr size_t log_truncation_length( void )
        {
            return ( 2u * EOL.length() ) + mc_log_truncation.length();
        }

        std::atomic_bool m_is_paused { false };

        Array<char> m_storage;
        Queue_MPSC<char> m_queue;

        Signal m_signal_log_paused;
        Observer_handler<Log> m_observer_queue_full;

        I_single_consumer_queue<char>& consumer_queue( void );

        Signal& log_paused_signal( void );
        bool is_paused( void ) const;
        bool try_resume( void );

        void push_line( String_view line );

        void async_on_queue_full( void );
    };

    constexpr size_t Log::line_buffer_size( void )
    {
        return mc_uptime_digits +
               mc_uptime_separator.length() +
               SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH +
               EOL.length() +
               1u;
    };

    ///
    /// @brief Stream-style logging macros backed by the exposed Log object.
    ///
    ///        Example: ILOG << "Temperature: " << temperature;
    ///
    ///        Define SOMA_CONFIG_ENABLE_DLOG before including this header to enable DLOG in
    ///        the current translation unit. Otherwise DLOG is compiled out.
#ifdef SOMA_CONFIG_ENABLE_DLOG
    #define DLOG                                                                         \
            if( SOMA::Log* p_log = SOMA::Log::get_exposed(); p_log != nullptr )          \
            if( SOMA::Array<char, SOMA::Log::line_buffer_size()> log_line_buffer; true ) \
            SOMA::Log::Text_log_output( *p_log,                                          \
                                        SOMA::Log::Log_severity_e::debug,                \
                                        nullptr,                                         \
                                        0,                                               \
                                        log_line_buffer )
#else
    #define DLOG \
            if constexpr( false ) ELOG
#endif

#define ILOG                                                                         \
        if( SOMA::Log * p_log = SOMA::Log::get_exposed(); p_log != nullptr )         \
        if( SOMA::Array<char, SOMA::Log::line_buffer_size()> log_line_buffer; true ) \
        SOMA::Log::Text_log_output( *p_log,                                          \
                                    SOMA::Log::Log_severity_e::info,                 \
                                    nullptr,                                         \
                                    0,                                               \
                                    log_line_buffer )

#define ELOG                                                                         \
        if( SOMA::Log * p_log = SOMA::Log::get_exposed(); p_log != nullptr )         \
        if( SOMA::Array<char, SOMA::Log::line_buffer_size()> log_line_buffer; true ) \
        SOMA::Log::Text_log_output( *p_log,                                          \
                                    SOMA::Log::Log_severity_e::error,                \
                                    nullptr,                                         \
                                    0,                                               \
                                    log_line_buffer )

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif
void SOMA_log( const char* p_format,
               ... ) SOMA_COMPILER_PRINTF_FORMAT( 1, 2 );
#ifdef __cplusplus
}
} // namespace SOMA
#endif

#endif // LOG_H_
