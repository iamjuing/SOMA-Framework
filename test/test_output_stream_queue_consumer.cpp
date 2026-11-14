// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <atomic>
#include <vector>

#include "test_common.h"
#include "soma/io/output_stream_queue_consumer.h"
#include "soma/containers/queue_spsc.h"
#include "soma/runtime/task_memory.h"

namespace
{
    using Consumer = Output_stream_queue_consumer<uint8_t>;

    class Test_output_stream: public I_output_stream
    {
    public:
        Test_output_stream(): I_output_stream( "output" ) {}

        Result_e write( Buffer_view<uint8_t> data ) override
        {
            const Result_e result = m_result.load();
            if( result != Result_e::unavailable )
            {
                m_received.insert( m_received.end(), data.begin(), data.end() );
            }
            m_calls.fetch_add( 1u );
            return result;
        }

        void complete_write( void ) { notify(); }

        std::atomic<Result_e> m_result { Result_e::unavailable };
        std::atomic_size_t m_calls { 0u };
        std::vector<uint8_t> m_received;
    };

    struct Fixture
    {
        explicit Fixture( Consumer::Unavailable_policy_e policy = Consumer::Unavailable_policy_e::discard,
                          uint32_t batching_ms = 0u )
            : task( "consumer", Task::Priority_e::high, memory ),
              consumer( task, queue, stream, batching_ms, 2u, policy ) {}

        ~Fixture() { task.stop(); }

        Task_memory<1000> memory;
        Event_task task;
        Queue_SPSC<uint8_t, 16> queue { "queue" };
        Test_output_stream stream;
        Consumer consumer;
    };
}

TEST( output_stream_queue_consumer, default_policy_discards_unavailable_chunks )
{
    Task_memory<1000> memory;
    Event_task task( "consumer", Task::Priority_e::high, memory );
    Queue_SPSC<uint8_t, 16> queue( "queue" );
    Test_output_stream stream;
    Consumer consumer( task, queue, stream, 0u, 2u );
    const uint8_t data[] = { 1, 2, 3, 4, 5 };
    queue.push( Buffer_view<uint8_t>( Array( data ) ) );

    task.start();
    const bool drained = wait_until_true( 1000u, [&]() { return queue.is_empty(); } );
    task.stop();

    ASSERT_TRUE( drained );
    EXPECT_EQ( stream.m_calls.load(), 3u );
    EXPECT_TRUE( stream.m_received.empty() );
}

TEST( output_stream_queue_consumer, retain_pauses_until_side_channel_resumes )
{
    Fixture fixture( Consumer::Unavailable_policy_e::retain );
    fixture.queue.push( 1u );
    fixture.task.start();
    ASSERT_EQ_IN_TIME( fixture.stream.m_calls.load(), 1u, 1000u );

    fixture.queue.push( 2u );
    fixture.stream.m_result.store( I_output_stream::Result_e::ready );
    Task::sleep( 30u );
    EXPECT_EQ( fixture.stream.m_calls.load(), 1u );
    EXPECT_EQ( fixture.queue.size(), 2u );

    fixture.consumer.resume();
    ASSERT_EQ_IN_TIME( fixture.queue.size(), 0u, 1000u );
    fixture.task.stop();

    EXPECT_EQ( fixture.stream.m_received, ( std::vector<uint8_t> { 1, 2 } ) );
}

TEST( output_stream_queue_consumer, unsuccessful_resume_pauses_again )
{
    Fixture fixture( Consumer::Unavailable_policy_e::retain );
    fixture.queue.push( 7u );
    fixture.task.start();
    ASSERT_EQ_IN_TIME( fixture.stream.m_calls.load(), 1u, 1000u );

    fixture.consumer.resume();
    ASSERT_EQ_IN_TIME( fixture.stream.m_calls.load(), 2u, 1000u );
    Task::sleep( 30u );
    EXPECT_EQ( fixture.stream.m_calls.load(), 2u );
    EXPECT_EQ( fixture.queue.size(), 1u );

    fixture.stream.m_result.store( I_output_stream::Result_e::ready );
    fixture.consumer.resume();
    ASSERT_EQ_IN_TIME( fixture.queue.size(), 0u, 1000u );
    fixture.task.stop();
    EXPECT_EQ( fixture.stream.m_received, ( std::vector<uint8_t> { 7 } ) );
}

TEST( output_stream_queue_consumer, resume_does_not_complete_an_in_flight_write )
{
    Fixture fixture( Consumer::Unavailable_policy_e::retain );
    fixture.stream.m_result.store( I_output_stream::Result_e::wait );
    fixture.queue.push( 1u );
    fixture.task.start();
    ASSERT_EQ_IN_TIME( fixture.stream.m_calls.load(), 1u, 1000u );

    fixture.queue.push( 2u );
    fixture.consumer.resume();
    Task::sleep( 30u );
    EXPECT_EQ( fixture.stream.m_calls.load(), 1u );
    EXPECT_EQ( fixture.queue.size(), 2u );

    fixture.stream.m_result.store( I_output_stream::Result_e::ready );
    fixture.stream.complete_write();
    ASSERT_EQ_IN_TIME( fixture.queue.size(), 0u, 1000u );
    fixture.task.stop();
    EXPECT_EQ( fixture.stream.m_received, ( std::vector<uint8_t> { 1, 2 } ) );
}

TEST( output_stream_queue_consumer, resume_bypasses_batching_delay )
{
    Fixture fixture( Consumer::Unavailable_policy_e::retain, 5000u );
    fixture.queue.push( 9u );
    fixture.task.start();
    ASSERT_EQ_IN_TIME( fixture.stream.m_calls.load(), 1u, 1000u );

    fixture.stream.m_result.store( I_output_stream::Result_e::ready );
    fixture.consumer.resume();
    ASSERT_EQ_IN_TIME( fixture.queue.size(), 0u, 1000u );
    fixture.task.stop();
    EXPECT_EQ( fixture.stream.m_received, ( std::vector<uint8_t> { 9 } ) );
}
