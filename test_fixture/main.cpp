// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <thread>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"

#include "gtest/gtest.h"

#include "soma/io/i_output_stream.h"
#include "soma/diagnostics/log_streamer.h"
#include "soma/runtime/event_task.h"
#include "soma/runtime/heap.h"
#include "soma/runtime/task_memory.h"

using namespace SOMA;

[[noreturn]] void soma_test_panic( String_view file_name,
                                   int line,
                                   String_view message )
{
    if( file_name.is_empty() && message.is_empty() )
    {
        std::fputs( "PANIC !!!\n", stderr );
    }
    else
    {
        std::fputs( "PANIC: ", stderr );
        if( file_name.is_empty() == false )
        {
            std::fwrite( file_name.buffer().array().storage(), 1u, file_name.length(), stderr );
        }
        std::fprintf( stderr, ":%d: ", line );
        if( message.is_empty() == false )
        {
            std::fwrite( message.buffer().array().storage(), 1u, message.length(), stderr );
        }
        std::fputc( '\n', stderr );
    }
    std::fflush( stderr );
    std::_Exit( EXIT_FAILURE );
}

static Array<uint8_t, 1u * 1024u * 1024u> heap_region_1;
static const Array heap_regions { heap_region_1 };

class File_output_stream: public I_output_stream
{
public:
    File_output_stream( String_view name )
        : I_output_stream( name )
    {
        // intentionally empty
    }
    ~File_output_stream()
    {
        std::string absolute_path = std::filesystem::absolute( log_file_name ).string();

        printf( "\nLog file stored in: %s\n", absolute_path.c_str() );
    }

    Result_e write( Buffer_view<uint8_t> data ) override
    {
        if( log_file.is_open() )
        {
            log_file.write( reinterpret_cast<const char*>( data.array().storage() ), data.size() );
        }

        return Result_e::ready;
    }

private:
    std::filesystem::path log_file_name { "unit_test_log.txt" };
    std::ofstream log_file { log_file_name };
};

class FreeRTOS_heap_check: public testing::EmptyTestEventListener
{
    void OnTestStart( const testing::TestInfo& test_info ) override
    {
        m_heap_free_bytes = static_cast<long long>( xPortGetFreeHeapSize() );
    }

    void OnTestEnd( const testing::TestInfo& test_info ) override
    {
        // check the FreeRTOS heap is the same as before the test
        long long memory_leak_bytes = labs( m_heap_free_bytes - static_cast<long long>( xPortGetFreeHeapSize() ) );

        if( memory_leak_bytes > 0 )
        {
            printf( "ERROR: FreeRTOS heap memory leak of %llu bytes detected!\n", memory_leak_bytes );

            exit( 1 );
        }
    }

    long long m_heap_free_bytes;
};

class FreeRTOS_task_check: public testing::EmptyTestEventListener
{
    std::vector<TaskStatus_t> task_status( void ) const
    {
        std::vector<TaskStatus_t> rv_task_status;

        vTaskSuspendAll();

        UBaseType_t task_cnt = uxTaskGetNumberOfTasks();

        rv_task_status.resize( task_cnt );

        task_cnt = uxTaskGetSystemState( rv_task_status.data(), task_cnt, nullptr );

        rv_task_status.resize( task_cnt );

        xTaskResumeAll();

        return rv_task_status;
    }

    void OnTestStart( const testing::TestInfo& test_info ) override
    {
        m_expected_task_handles.clear();

        for( const TaskStatus_t& task: task_status() )
        {
            m_expected_task_handles.insert( task.xHandle );
        }
    }

    void OnTestEnd( const testing::TestInfo& test_info ) override
    {
        // let the FreeRTOS to settle down
        Sleep( 1 );

        const std::vector<TaskStatus_t> current_task_status = task_status();

        // check if there is an unexpected FreeRTOS task

        bool result = true;

        for( const TaskStatus_t& task: current_task_status )
        {
            if( m_expected_task_handles.find( task.xHandle ) == m_expected_task_handles.end() )
            // unexpected task detected
            {
                result = false;

                printf( "Unexpected FreeRTOS task detected after finished test: %s\n", task.pcTaskName );
            }
        }

        if( ( result ) &&
            ( current_task_status.size() != m_expected_task_handles.size() ) )
        // not all expected tasks are running
        {
            result = false;
        }

        if( result == false )
        {
            exit( 1 );
        }
    }

    std::set<TaskHandle_t> m_expected_task_handles;
};

static void gtest_task( void* p_task_param )
{
    int exit_code = RUN_ALL_TESTS();

    exit( exit_code );
}

void start( void )
{
    // intentionally empty
}

int main( int argc,
          char** argv )
{
    Heap::initialize( heap_regions );

    static File_output_stream log_file_stream { "log_file" };
    static Array<char, 100 * 1024> log_buffer;
    static Log log( log_buffer );
    static Task_memory<1000> m_task_storage;
    static Event_task log_streamer_task { "log_streamer", Task::Priority_e::idle, m_task_storage };
    static Log_streamer log_streamer( log_streamer_task, log, 20u, log_file_stream );

    ::testing::InitGoogleTest( &argc, argv );

    // keep the order
    ::testing::UnitTest::GetInstance()->listeners().Append( new FreeRTOS_heap_check );
    ::testing::UnitTest::GetInstance()->listeners().Append( new FreeRTOS_task_check );

    // Google Test framework is running as a regular FreeRTOS task, so it can properly interact with FreeRTOS tasks under test
    if( xTaskCreate( gtest_task,
                     "gtest",
                     10000,
                     nullptr,
                     static_cast<UBaseType_t>( Task::Priority_e::high ),
                     nullptr ) == pdPASS )
    {
        log.expose();

        log_streamer_task.start();

        vTaskStartScheduler();

        // unreachable
    }

    return 1; // failed
}
