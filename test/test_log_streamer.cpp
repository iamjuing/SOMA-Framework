// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#define SOMA_CONFIG_ENABLE_DLOG

#include <cstring>

#include "test_common.h"

#include "soma/config.h"
#include "soma/diagnostics/log_streamer.h"
#include "soma/io/i_output_stream.h"
#include "soma/runtime/event_task.h"
#include "soma/runtime/task.h"
#include "soma/runtime/task_memory.h"

using namespace SOMA;

namespace
{
    const size_t c_timestamp_size = 10 + 2; // +2 for delimiter

    class Scoped_log_exposure
    {
    public:
        explicit Scoped_log_exposure( Log& log )
            : m_log( log ),
              mp_previous_log( Log::get_exposed() )
        {
            m_log.expose();
        }

        ~Scoped_log_exposure( void )
        {
            m_log.hide();

            if( mp_previous_log != nullptr )
            {
                mp_previous_log->expose();
            }
        }

    private:
        Log& m_log;
        Log* mp_previous_log;
    };

    // Test-specific logging macros that work with a local Log instance
    // instead of the globally exposed one
    #define TEST_DLOG( log )                                                 \
            if( Array<char, Log::line_buffer_size()> log_line_buffer; true ) \
            Log::Text_log_output( ( log ),                                   \
                                  Log::Log_severity_e::debug,                \
                                  nullptr,                                   \
                                  0,                                         \
                                  log_line_buffer )

    #define TEST_ILOG( log )                                                 \
            if( Array<char, Log::line_buffer_size()> log_line_buffer; true ) \
            Log::Text_log_output( ( log ),                                   \
                                  Log::Log_severity_e::info,                 \
                                  nullptr,                                   \
                                  0,                                         \
                                  log_line_buffer )

    #define TEST_ELOG( log, file, line )                                     \
            if( Array<char, Log::line_buffer_size()> log_line_buffer; true ) \
            Log::Text_log_output( ( log ),                                   \
                                  Log::Log_severity_e::error,                \
                                  file,                                      \
                                  line,                                      \
                                  log_line_buffer )
};

// class mocking the output stream
class Mock_output_stream: public I_output_stream
{
public:
    Mock_output_stream( const char* p_name,
                        size_t rx_buf_size )
        : I_output_stream( p_name ),
          mp_name( p_name ),
          m_rx_data_cnt( 0 )
    {
        m_rx_buffer.resize( rx_buf_size );
        m_rx_buffer.clear();
    }

    Result_e write( Buffer_view<uint8_t> data ) override
    {
        m_rx_buffer.append( reinterpret_cast<const char*>( data.array().storage() ), data.size() );

        m_rx_data_cnt.fetch_add( data.size(), std::memory_order_release );

        return Result_e::ready;
    }

    void empty( void )
    {
        m_rx_buffer.clear();
        m_rx_data_cnt.store( 0, std::memory_order_release );
    }

    const uint8_t* get_buffer( void ) const
    {
        return reinterpret_cast<const uint8_t*>( m_rx_buffer.c_str() );
    }

    size_t get_rx_data_cnt( void ) const
    {
        return m_rx_data_cnt.load( std::memory_order_acquire );
    }

    bool contains( const char* p_text ) const
    {
        return m_rx_buffer.find( p_text ) != std::string::npos;
    }

    size_t occurrence_count( const char* p_text ) const
    {
        size_t count = 0u;
        size_t position = 0u;
        const size_t text_length = strlen( p_text );

        while( ( position = m_rx_buffer.find( p_text, position ) ) != std::string::npos )
        {
            ++count;
            position += text_length;
        }

        return count;
    }

    void get_received_lines( std::vector<std::string>& received_lines )
    {
        std::string delimiter = "\r\n";
        size_t start = 0;
        size_t pos;

        received_lines.clear();

        while( ( pos = m_rx_buffer.find( delimiter, start ) ) != std::string::npos )
        {
            std::string line = m_rx_buffer.substr( start, pos - start + delimiter.length() );

            if( line != "[SOMA] <LOG TRUNCATED>\r\n" )
            // filter out the timestamp (and preserve color codes if present)
            {
                // Check if line starts with ANSI color code (ESC [ digits m)
                if( line.length() > 0 && line[0] == '\033' )
                {
                    // Find the end of the color code (the 'm' character) within first 10 chars
                    // ANSI color codes are typically \033[##m which is at most 6 characters
                    size_t color_end = line.find( 'm', 0 );

                    if( color_end != std::string::npos && color_end < 10 )
                    {
                        // Extract color code and skip timestamp
                        std::string color_code = line.substr( 0, color_end + 1 );

                        line = color_code + line.substr( color_end + 1 + c_timestamp_size );
                    }
                }
                else
                {
                    line = line.substr( c_timestamp_size );
                }
            }

            received_lines.push_back( line );

            start = pos + delimiter.length();
        }

        if( start < m_rx_buffer.length() )
        {
            received_lines.push_back( m_rx_buffer.substr( start ) );
        }
    }

    size_t get_expected_data_count( std::vector<std::string>& extected_lines )
    {
        size_t rv_char_num = 0;

        for( const auto& line: extected_lines )
        {
            rv_char_num += line.length();

            if( line != "[SOMA] <LOG TRUNCATED>\r\n" )
            {
                rv_char_num += c_timestamp_size;
            }
        }

        return rv_char_num;
    }

private:
    const char* mp_name;
    std::string m_rx_buffer;
    std::atomic_size_t m_rx_data_cnt;
};

TEST( log_streamer, public_producer_interfaces_remain_available )
{
    Mock_output_stream stream( "test_stream", 2000 );
    Array<char, 1000> log_buffer;
    Log tested_log( log_buffer );
    Scoped_log_exposure log_exposure( tested_log );

    Task_memory<1000> task_storage;
    Event_task task { "task", Task::Priority_e::high, task_storage };
    Log_streamer tested_log_streamer( task, tested_log, 20u, stream );

    DLOG << "debug macro";
    ILOG << "info macro";
    ELOG << "error macro";
    SOMA_log( "printf-style %d", 42 );

    set_ISR_context( true );
    ILOG << "ISR macro";
    set_ISR_context( false );

    task.start();

    ASSERT_EQ_IN_TIME( stream.contains( "debug macro" ), true, 3000 );
    EXPECT_TRUE( stream.contains( "info macro" ) );
    EXPECT_TRUE( stream.contains( "error macro" ) );
    EXPECT_TRUE( stream.contains( "printf-style 42" ) );
    EXPECT_TRUE( stream.contains( "ISR macro" ) );
    EXPECT_TRUE( stream.contains( "[SOMA] \033[32mStarting task: 'task'" ) );
    EXPECT_FALSE( stream.contains( "[SOMA] info macro" ) );
}

TEST( log_streamer, buffers_logs_before_consumer_is_constructed )
{
    constexpr char c_early_message[] = "logged before application construction";

    Mock_output_stream stream( "test_stream", 1000 );
    Array<char, 200> log_buffer;
    Log tested_log( log_buffer );

    TEST_ILOG( tested_log ) << c_early_message;

    Task_memory<1000> task_storage;
    Event_task task { "task", Task::Priority_e::high, task_storage };
    Log_streamer tested_log_streamer( task, tested_log, 20u, stream );

    task.start();

    ASSERT_EQ_IN_TIME( stream.contains( c_early_message ), true, 3000 );
}

TEST( log_streamer, recovers_full_log_before_consumer_is_constructed )
{
    Mock_output_stream stream( "test_stream", 2000 );
    Array<char, 40> log_buffer;
    Log tested_log( log_buffer );

    TEST_ILOG( tested_log ) << "This startup message is deliberately longer than the remaining log capacity";

    Task_memory<1000> task_storage;
    Event_task task { "task", Task::Priority_e::high, task_storage };
    Log_streamer tested_log_streamer( task, tested_log, 20u, stream );

    task.start();

    ASSERT_EQ_IN_TIME( stream.occurrence_count( "<LOG TRUNCATED>" ), 1u, 3000 );

    TEST_ILOG( tested_log ) << "logging resumed";

    ASSERT_EQ_IN_TIME( stream.contains( "logging resumed" ), true, 3000 );
    EXPECT_EQ( stream.occurrence_count( "<LOG TRUNCATED>" ), 1u );
}

TEST( log_streamer, recovers_full_log_between_construction_and_task_start )
{
    Mock_output_stream stream( "test_stream", 2000 );
    Array<char, 40> log_buffer;
    Log tested_log( log_buffer );

    Task_memory<1000> task_storage;
    Event_task task { "task", Task::Priority_e::high, task_storage };
    Log_streamer tested_log_streamer( task, tested_log, 20u, stream );

    TEST_ILOG( tested_log ) << "This message deliberately fills the log before the consumer task starts";

    task.start();

    ASSERT_EQ_IN_TIME( stream.occurrence_count( "<LOG TRUNCATED>" ), 1u, 3000 );

    TEST_ILOG( tested_log ) << "logging resumed";

    ASSERT_EQ_IN_TIME( stream.contains( "logging resumed" ), true, 3000 );
    EXPECT_EQ( stream.occurrence_count( "<LOG TRUNCATED>" ), 1u );
}

TEST( log_streamer, truncation_pause_begins_when_consumer_task_starts )
{
    Mock_output_stream stream( "test_stream", 2000 );
    Array<char, 40> log_buffer;
    Log tested_log( log_buffer );

    TEST_ILOG( tested_log ) << "This startup message deliberately fills the log";

    Task_memory<1000> task_storage;
    Event_task task { "task", Task::Priority_e::high, task_storage };
    Log_streamer tested_log_streamer( task, tested_log, 300u, stream );

    // Construction time must not consume the truncation pause.
    Task::sleep( 400u );
    task.start();

    Task::sleep( 100u );
    EXPECT_EQ( stream.occurrence_count( "<LOG TRUNCATED>" ), 0u );
    ASSERT_EQ_IN_TIME( stream.occurrence_count( "<LOG TRUNCATED>" ), 1u, 3000 );
}

TEST( log_streamer, buffers_logs_between_consumers )
{
    Mock_output_stream stream( "test_stream", 2000 );
    Array<char, 200> log_buffer;
    Log tested_log( log_buffer );

    {
        Task_memory<1000> task_storage;
        Event_task task { "first_logger", Task::Priority_e::high, task_storage };
        Log_streamer log_streamer( task, tested_log, 20u, stream );

        task.start();
        ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), 10u, 3000 );
    }

    TEST_ILOG( tested_log ) << "logged without a consumer";

    {
        Task_memory<1000> task_storage;
        Event_task task { "second_logger", Task::Priority_e::high, task_storage };
        Log_streamer log_streamer( task, tested_log, 20u, stream );

        task.start();
        ASSERT_EQ_IN_TIME( stream.contains( "logged without a consumer" ), true, 3000 );
    }
}

TEST( log_streamer, logging_to_stream )
{
    std::vector<std::string> result_lines;
    std::vector<std::string> received_lines;

    Mock_output_stream stream( "test_stream", 1000 );

    Array<char, 200> log_buffer;
    Task_memory<1000> m_task_storage;
    Event_task task { "task", Task::Priority_e::high, m_task_storage };
    Log tested_log( log_buffer );
    Log_streamer tested_log_streamer( task, tested_log, 500u, stream );

    task.start();

    // clean the initial log data
    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), 10, 3000 );
    stream.empty();

    TEST_DLOG( tested_log ) << "hydrogen";
    TEST_DLOG( tested_log ) << "oxid";
    TEST_DLOG( tested_log ) << 42;
    TEST_DLOG( tested_log ) << precision( 1 ) << 42.0;

    result_lines = { "hydrogen\r\n",
                     "oxid\r\n",
                     "42\r\n",
                     "42.0\r\n" };

    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), stream.get_expected_data_count( result_lines ), 3000 );

    stream.get_received_lines( received_lines );

    ASSERT_EQ( received_lines, result_lines );
}

TEST( log_streamer, line_truncation )
{
    std::vector<std::string> received_lines;
    std::vector<std::string> result_lines;

    const std::string truncation_marker { " <...>" };
    const std::string long_message( SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH + 7u, 'a' );
    const std::string message_at_limit( SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH, 'b' );
    const std::string message_above_limit( SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH + 1u, 'c' );
    const size_t retained_length = SOMA_CONFIG_MAX_LOG_MESSAGE_LENGTH - truncation_marker.length();

    Mock_output_stream stream( "test_stream", 3u * Log::line_buffer_size() );

    Array<char, 3u * Log::line_buffer_size()> log_buffer;
    Task_memory<1000> m_task_storage;
    Event_task task { "task", Task::Priority_e::high, m_task_storage };
    Log tested_log( log_buffer );
    Log_streamer tested_log_streamer( task, tested_log, 500u, stream );

    task.start();

    // clean the initial log data
    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), 10, 3000 );
    stream.empty();

    TEST_DLOG( tested_log ) << "first";
    TEST_DLOG( tested_log ) << long_message.c_str();
    TEST_DLOG( tested_log ) << "last";

    result_lines = { "first\r\n",
                     long_message.substr( 0u, retained_length ) + truncation_marker + "\r\n",
                     "last\r\n" };

    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), stream.get_expected_data_count( result_lines ), 3000 );

    stream.get_received_lines( received_lines );

    ASSERT_EQ( received_lines, result_lines );

    stream.empty();

    // A message exactly at the configured limit must remain intact.
    TEST_DLOG( tested_log ) << message_at_limit.c_str();

    result_lines = { message_at_limit + "\r\n" };

    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), stream.get_expected_data_count( result_lines ), 3000 );

    stream.get_received_lines( received_lines );

    ASSERT_EQ( received_lines, result_lines );

    stream.empty();

    // One character over the configured limit must trigger truncation.
    TEST_DLOG( tested_log ) << message_above_limit.c_str();

    result_lines = { message_above_limit.substr( 0u, retained_length ) + truncation_marker + "\r\n" };

    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), stream.get_expected_data_count( result_lines ), 3000 );

    stream.get_received_lines( received_lines );

    ASSERT_EQ( received_lines, result_lines );
}

TEST( log_streamer, log_truncation )
{
    std::vector<std::string> result_lines;
    std::vector<std::string> received_lines;

    Mock_output_stream stream( "test_stream", 1000 );

    Array<char, 32> log_buffer;
    Task_memory<1000> m_task_storage;
    Event_task task { "task", Task::Priority_e::high, m_task_storage };
    Log tested_log( log_buffer );
    Log_streamer tested_log_streamer( task, tested_log, 100u, stream );

    task.start();

    // clean the initial log data
    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), 10, 3000 );
    stream.empty();

    TEST_DLOG( tested_log ) << "first second third fourth fifth sixth seventh";

    result_lines = { "first second third f\r\n",
                     "[SOMA] <LOG TRUNCATED>\r\n" };

    ASSERT_EQ_IN_TIME( ( stream.get_received_lines( received_lines ), received_lines ), result_lines, 3000 );

    stream.empty();

    TEST_DLOG( tested_log ) << "eighth";

    result_lines = { "eighth\r\n" };

    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), stream.get_expected_data_count( result_lines ), 3000 );
    stream.get_received_lines( received_lines );
    ASSERT_EQ( received_lines, result_lines );
}

namespace
{
    const size_t c_producer_cnt = 40;
    const size_t c_log_line_cnt = 1'000'000;
    const char c_log_line[] = "This is a testing log line that can not be screwed up in multiple log producer scenario.";
    const size_t c_logger_buffer_size_B = c_producer_cnt *
                                          c_log_line_cnt *
                                          ( c_timestamp_size + sizeof( c_log_line ) - 1 + 2 ); // -1 for NULL, +2 for new lines
}

void task_log_producer( void* p_task_param )
{
    ASSERT_NE( SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL + 1 ), 0 );

    if( p_task_param )
    {
        Log* p_log = reinterpret_cast<Log*>( p_task_param );

        for( size_t i = 0; i < c_log_line_cnt; ++i )
        {
            TEST_DLOG( *p_log ) << c_log_line;
        }
    }

    // suspend to indicate the job is finished
    vTaskSuspend( nullptr );
}

TEST( log_streamer, log_multi_thread_consistency )
{
    // create, initialize and start the log_streamer (allocate on heap as it can be huge)
    auto p_stream = std::make_unique<Mock_output_stream>( "test_stream", c_logger_buffer_size_B );

    auto p_buffer = std::make_unique<Array<char, c_logger_buffer_size_B + ( 3u * Log::line_buffer_size() )>>();
    Task_memory<1000> m_task_storage;
    Event_task task { "task", Task::Priority_e::high, m_task_storage };
    Log tested_log( *p_buffer );
    Log_streamer tested_log_streamer( task, tested_log, 500u, *p_stream );

    task.start();

    // clean the initial log data
    ASSERT_EQ_IN_TIME( p_stream->get_rx_data_cnt(), 10, 3000 );
    p_stream->empty();

    TaskHandle_t task_handles[c_producer_cnt];

    // suspend the FreeRTOS scheduler to create all producers before they start
    // otherwise, the created task would preempt this thread and all would eventually run sequencially
    vTaskSuspendAll();

    for( size_t i = 0; i < c_producer_cnt; ++i )
    {
        ASSERT_EQ( xTaskCreate( task_log_producer,
                                "producer",
                                500,
                                &tested_log,
                                static_cast<UBaseType_t>( Task::Priority_e::high ),
                                &task_handles[i] ),
                   pdPASS );
    }

    xTaskResumeAll();

    // wait until all the threads suspends themselves (indication of done)
    ASSERT_TRUE( wait_until_true( 5 * 60 * 1000,
                                  [&]( ) {
                                      bool producers_finished = true;

                                      for( size_t i = 0; i < c_producer_cnt; ++i )
                                      {
                                          if( eTaskGetState( task_handles[i] ) != eTaskState::eSuspended )
                                          {
                                              producers_finished = false;
                                              break;
                                          }
                                      }

                                      return producers_finished;
                                  } ) );

    // cleanup all the producer tasks
    for( size_t i = 0; i < c_producer_cnt; ++i )
    {
        vTaskDelete( task_handles[i] );
    }

    // wait until the log_streamer flushes all the data
    ASSERT_EQ_IN_TIME( p_stream->get_rx_data_cnt(), c_logger_buffer_size_B, 5 * 60 * 1000 );

    auto p_stream_buffer = p_stream->get_buffer();

    for( size_t i = 0; i < ( c_producer_cnt * c_log_line_cnt ) ; ++i )
    {
        // skip the timestamp
        p_stream_buffer += c_timestamp_size;

        ASSERT_MEM_EQ( p_stream_buffer, c_log_line, sizeof( c_log_line ) - 1 );
        p_stream_buffer += sizeof( c_log_line ) - 1;

        ASSERT_MEM_EQ( p_stream_buffer, "\r\n", 2 );
        p_stream_buffer += 2;
    }

    }

TEST( log_streamer, log_line_location )
{
    std::vector<std::string> result_lines;
    std::vector<std::string> received_lines;

    Mock_output_stream stream( "test_stream", 1000 );

    Array<char, 1000> log_buffer;
    Task_memory<1000> m_task_storage;
    Event_task task { "task", Task::Priority_e::high, m_task_storage };
    Log tested_log( log_buffer );
    Log_streamer tested_log_streamer( task, tested_log, 500u, stream );

    task.start();

    // clean the initial log data
    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), 10, 3000 );
    stream.empty();

    TEST_ELOG( tested_log, "test_file_a.cpp", 42 ) << "hydrogen";
    TEST_ELOG( tested_log, "test_file_b.cpp", 52 ) << "oxid";
    TEST_ELOG( tested_log, "test_file_c.cpp", 62 ) << 42;
    TEST_ELOG( tested_log, "test_file_d.cpp", 72 ) << precision( 1 ) << 42.0;

    result_lines = { "\033[31mtest_file_a.cpp:42: hydrogen\033[0m\r\n",
                     "\033[31mtest_file_b.cpp:52: oxid\033[0m\r\n",
                     "\033[31mtest_file_c.cpp:62: 42\033[0m\r\n",
                     "\033[31mtest_file_d.cpp:72: 42.0\033[0m\r\n" };

    ASSERT_EQ_IN_TIME( stream.get_rx_data_cnt(), stream.get_expected_data_count( result_lines ), 3000 );

    stream.get_received_lines( received_lines );

    ASSERT_EQ( received_lines, result_lines );
}
