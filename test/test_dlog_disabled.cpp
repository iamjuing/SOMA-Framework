// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "gtest/gtest.h"

#include "soma/diagnostics/log.h"

using namespace SOMA;

TEST( log, disabled_DLOG_is_a_no_op )
{
    int evaluation_count = 0;

    DLOG << ++evaluation_count;

    EXPECT_EQ( evaluation_count, 0 );
}
