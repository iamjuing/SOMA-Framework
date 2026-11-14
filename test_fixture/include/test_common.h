// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TEST_COMMON_H_
#define SOMA_TEST_COMMON_H_

#include <stdint.h>
#include <functional>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "soma/containers/buffer_view.h"

using namespace ::testing;
using namespace SOMA;

void set_ISR_context( bool is_ISR );

void busy_loop( size_t time_ms );

bool wait_until_true( size_t timeout_ms,
                      std::function<bool()> condition );

#define ASSERT_EQ_IN_TIME( v1, v2, t ) ASSERT_TRUE( wait_until_true( ( t ), [&]() { return ( ( v1 ) == ( v2 ) ); } ) )

#define ASSERT_MEM_EQ( p1, p2, n ) ASSERT_EQ( memcmp( reinterpret_cast<const void*>( p1 ), \
                                                      reinterpret_cast<const void*>( p2 ), \
                                                      n ),                                 \
                                              0 )

#define EXPECT_BUFFER_EQ( buffer, ... )                                       \
        {                                                                     \
            uint8_t expected_storage[] = { __VA_ARGS__ };                     \
            Buffer_view<uint8_t> expected_data { Array( expected_storage ) }; \
            EXPECT_EQ( buffer, expected_data );                               \
        }

#endif // TEST_COMMON_H_
