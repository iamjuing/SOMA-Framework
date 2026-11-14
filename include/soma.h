// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_H_
#define SOMA_H_

// Core
#include "soma/array.h"
#include "soma/assert.h"
#include "soma/i_object_enumerator.h"
#include "soma/i_observable.h"
#include "soma/observer.h"
#include "soma/observer_handler.h"
#include "soma/reverse_iterator.h"
#include "soma/time.h"

// Containers
#include "soma/containers/buffer.h"
#include "soma/containers/buffer_view.h"
#include "soma/containers/i_queue.h"
#include "soma/containers/i_queue_profile.h"
#include "soma/containers/i_single_consumer_queue.h"
#include "soma/containers/i_single_producer_queue.h"
#include "soma/containers/linked_list.h"
#include "soma/containers/memory_pool.h"
#include "soma/containers/queue_mpsc.h"
#include "soma/containers/queue_spsc.h"
#include "soma/containers/ring_buffer.h"

// Diagnostics
#include "soma/diagnostics/log.h"
#include "soma/diagnostics/log_streamer.h"
#include "soma/diagnostics/memory_profiler.h"
#include "soma/diagnostics/task_profiler.h"

// I/O
#include "soma/io/blocking_input_stream.h"
#include "soma/io/blocking_output_stream.h"
#include "soma/io/buffered_input_stream.h"
#include "soma/io/buffered_output_stream.h"
#include "soma/io/i_blocking_input_stream.h"
#include "soma/io/i_blocking_output_stream.h"
#include "soma/io/i_input_stream.h"
#include "soma/io/i_output_stream.h"
#include "soma/io/input_stream_queue_producer.h"
#include "soma/io/null_output_stream.h"
#include "soma/io/output_stream_queue_consumer.h"

// Variables
#include "soma/variables/b_variable.h"
#include "soma/variables/bounded_variable.h"
#include "soma/variables/overridable_bounded_variable.h"
#include "soma/variables/overridable_variable.h"
#include "soma/variables/variable.h"
#include "soma/variables/variable_observer_handler.h"

// Runtime
#include "soma/runtime/application_factory.h"
#include "soma/runtime/b_application.h"
#include "soma/runtime/b_task_event.h"
#include "soma/runtime/b_task_timer.h"
#include "soma/runtime/binary_semaphore.h"
#include "soma/runtime/critical_section.h"
#include "soma/runtime/event_task.h"
#include "soma/runtime/heap.h"
#include "soma/runtime/mutex.h"
#include "soma/runtime/mutex_guard.h"
#include "soma/runtime/recursive_mutex.h"
#include "soma/runtime/run.h"
#include "soma/runtime/scheduler_suspension_guard.h"
#include "soma/runtime/semaphore.h"
#include "soma/runtime/signal.h"
#include "soma/runtime/sleeping_spin_lock.h"
#include "soma/runtime/sleeping_spin_lock_guard.h"
#include "soma/runtime/synchronization_semaphore.h"
#include "soma/runtime/task.h"
#include "soma/runtime/task_event.h"
#include "soma/runtime/task_memory.h"
#include "soma/runtime/task_startup_event.h"
#include "soma/runtime/task_timer.h"

// Text
#include "soma/text/string.h"
#include "soma/text/string_table_generator.h"
#include "soma/text/string_view.h"
#include "soma/text/text_input.h"
#include "soma/text/text_output.h"
#include "soma/text/text_string_input.h"
#include "soma/text/text_string_output.h"

#endif // SOMA_H_
