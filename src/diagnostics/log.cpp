// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <inttypes.h>

#include "soma/diagnostics/log.h"

#include "soma/runtime/event_task.h"
#include "soma/text/string.h"
#include "soma/time.h"

namespace SOMA
{
    Log::Log( Array<char> storage )
        : m_storage( storage ),
          m_queue( "log", storage ),
          m_observer_queue_full( *this, &Log::async_on_queue_full )
    {
        ASSERT( SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH > mc_str_line_trunc.length() );
        ASSERT( m_queue.capacity() > log_truncation_length() );

        m_queue.signal_queue_full().subscribe( m_observer_queue_full );

        // Reset terminal color and separate this boot's log from previous output.
        m_queue.push( Color::standard );
        m_queue.push( EOL );
        m_queue.push( EOL );
        m_queue.push( EOL );
    }

    Log::~Log()
    {
        m_queue.signal_queue_full().unsubscribe( m_observer_queue_full );
    }

    void Log::expose( void )
    {
        mp_log = this;
    }

    void Log::hide( void )
    {
        if( mp_log == this )
        {
            mp_log = nullptr;
        }
    }

    Log* Log::get_exposed( void )
    {
        return mp_log;
    }

    Array<Array<const char>, 2> Log::raw_content( void ) const
    {
        const auto first = m_queue.view_items();
        const size_t count = m_queue.size();

        return Array<Array<const char>, 2> { first.array(),
                                             Array<const char> { m_storage.storage(), count - first.size() } };
    }

    size_t Log::capacity( void ) const
    {
        return m_queue.capacity();
    }

    void Log::log( const char* p_format,
                   va_list args )
    {
        Array<char, Log::line_buffer_size()> line_buffer;

        Text_log_output output( *this,
                                Log_severity_e::debug,
                                nullptr,
                                0,
                                line_buffer );

        output.vformat( p_format, args );
    }

    I_single_consumer_queue<char>& Log::consumer_queue( void )
    {
        return m_queue;
    }

    Signal& Log::log_paused_signal( void )
    {
        return m_signal_log_paused;
    }

    bool Log::is_paused( void ) const
    {
        return m_is_paused.load( std::memory_order_relaxed );
    }

    void Log::async_on_queue_full( void )
    {
        if( m_is_paused.exchange( true, std::memory_order_relaxed ) == false )
        {
            m_signal_log_paused.emit();
        }
    }

    bool Log::try_resume( void )
    {
        bool rv_resumed = false;

        if( is_paused() &&
            ( m_queue.space() > log_truncation_length() ) )
        {
            const size_t stored = m_queue.push( EOL ) +
                                  m_queue.push( mc_log_truncation ) +
                                  m_queue.push( EOL );

            if( stored == log_truncation_length() )
            {
                m_is_paused.store( false, std::memory_order_relaxed );
                rv_resumed = true;
            }
        }

        return rv_resumed;
    }

    void Log::push_line( String_view line )
    {
        if( is_paused() == false )
        {
            m_queue.push( line );
        }
    }

    Log::Text_log_output::Stream::Stream( String& output_text )
        : m_output_text( output_text )
    {
        // intentionally empty
    }

    Log::Text_log_output::Stream::~Stream( void ) = default;

    bool Log::Text_log_output::Stream::write( Buffer_view<uint8_t> data )
    {
        const bool rv_success = data.size() <= m_output_text.space();

        m_output_text += String_view( data );

        return rv_success;
    }

    Log::Text_log_output::Text_log_output( Log& log,
                                           Log_severity_e severity,
                                           const char* p_file_name,
                                           int line,
                                           Array<char> line_buffer )
        : Text_output( m_stream ),
          m_log( log ),
          m_line_str( Buffer( line_buffer ) ),
          m_stream( m_line_str ),
          m_severity( severity )
    {
        ASSERT( xTaskGetSchedulerState() != taskSCHEDULER_SUSPENDED );

        if( m_severity == Log_severity_e::error )
        {
            m_line_str += Color::red;
        }

        m_line_str.format( "%*" PRIu32, static_cast<int>( mc_uptime_digits ), Time::uptime_ms() );
        m_line_str += mc_uptime_separator;

        if( p_file_name )
        {
            m_line_str += p_file_name;
            m_line_str += ":";
            m_line_str += line;
            m_line_str += ": ";
        }
    }

    Log::Text_log_output::~Text_log_output()
    {
        while( m_line_str.is_empty() == false )
        {
            String_view view = m_line_str.view();
            char last_char = view[view.length() - 1u];

            if( ( last_char == '\n' ) || ( last_char == '\r' ) )
            {
                m_line_str -= 1u;
            }
            else
            {
                break;
            }
        }

        size_t color_reset_length = 0u;

        if( m_severity == Log_severity_e::error )
        {
            color_reset_length = Color::standard.length();
        }

        size_t required_space = EOL.length() + color_reset_length;

        if( m_line_str.space() < required_space )
        {
            m_line_str -= required_space - m_line_str.space();
            m_line_str -= mc_str_line_trunc.length();
            m_line_str += mc_str_line_trunc;
        }

        if( m_severity == Log_severity_e::error )
        {
            m_line_str += Color::standard;
        }

        m_line_str += EOL;
        m_log.push_line( m_line_str.view() );
    }

    void Log::Text_log_output::vformat( const char* p_format,
                                        va_list args )
    {
        m_line_str.vformat( p_format, args );
    }

    extern "C" void SOMA_log( const char* p_format,
                              ... )
    {
        Log* p_log = Log::get_exposed();

        if( p_log != nullptr )
        {
            va_list args;

            va_start( args, p_format );
            p_log->log( p_format, args );
            va_end( args );
        }
    }
} // namespace SOMA
