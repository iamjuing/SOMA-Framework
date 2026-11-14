// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"
#include "mock_variable_observer.h"

#include "soma/variables/bounded_variable.h"
#include "soma/variables/variable_observer_handler.h"

#include <limits>

using namespace SOMA;

TEST( bounded_variable, constructor_min_max_only )
{
    Bounded_variable<int> param( 10, 100 );

    // Constructor with min/max should initialize with min value
    ASSERT_EQ( param.get(), 10 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
}

TEST( bounded_variable, constructor_with_value_within_bounds )
{
    Bounded_variable<int> param( 50, 10, 100 );

    ASSERT_EQ( param.get(), 50 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
}

TEST( bounded_variable, constructor_with_value_at_min_bound )
{
    Bounded_variable<int> param( 10, 10, 100 );

    ASSERT_EQ( param.get(), 10 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
}

TEST( bounded_variable, constructor_with_value_at_max_bound )
{
    Bounded_variable<int> param( 100, 10, 100 );

    ASSERT_EQ( param.get(), 100 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
}

TEST( bounded_variable, init_within_bounds )
{
    Bounded_variable<int> param( 10, 100 );

    param.init( 50 );

    ASSERT_EQ( param.get(), 50 );
}

TEST( bounded_variable, init_at_min_bound )
{
    Bounded_variable<int> param( 10, 100 );

    param.init( 10 );

    ASSERT_EQ( param.get(), 10 );
}

TEST( bounded_variable, init_at_max_bound )
{
    Bounded_variable<int> param( 10, 100 );

    param.init( 100 );

    ASSERT_EQ( param.get(), 100 );
}

TEST( bounded_variable, init_below_min_bound_clamped )
{
    Bounded_variable<int> param( 10, 100 );

    param.init( 5 );

    // Value should be clamped to minimum
    ASSERT_EQ( param.get(), 10 );
}

TEST( bounded_variable, init_above_max_bound_clamped )
{
    Bounded_variable<int> param( 10, 100 );

    param.init( 150 );

    // Value should be clamped to maximum
    ASSERT_EQ( param.get(), 100 );
}

TEST( bounded_variable, assignment_within_bounds )
{
    Bounded_variable<int> param( 10, 100 );

    param = 75;

    ASSERT_EQ( param.get(), 75 );
}

TEST( bounded_variable, assignment_at_min_bound )
{
    Bounded_variable<int> param( 10, 100 );

    param = 10;

    ASSERT_EQ( param.get(), 10 );
}

TEST( bounded_variable, assignment_at_max_bound )
{
    Bounded_variable<int> param( 10, 100 );

    param = 100;

    ASSERT_EQ( param.get(), 100 );
}

TEST( bounded_variable, assignment_below_min_bound_clamped )
{
    Bounded_variable<int> param( 10, 100 );

    param = -5;

    // Value should be clamped to minimum
    ASSERT_EQ( param.get(), 10 );
}

TEST( bounded_variable, assignment_above_max_bound_clamped )
{
    Bounded_variable<int> param( 10, 100 );

    param = 200;

    // Value should be clamped to maximum
    ASSERT_EQ( param.get(), 100 );
}

TEST( bounded_variable, assignment_from_another_variable )
{
    Bounded_variable<int> param1( 50, 10, 100 );
    Bounded_variable<int> param2( 10, 100 );

    param2 = param1;

    ASSERT_EQ( param2.get(), 50 );
}

TEST( bounded_variable, assignment_from_another_variable_clamped )
{
    Bounded_variable<int> param1( 80, 70, 90 );  // Different bounds
    Bounded_variable<int> param2( 10, 60 );      // Max bound is 60

    param2 = param1;  // 80 should be clamped to 60

    ASSERT_EQ( param2.get(), 60 );
}

TEST( bounded_variable, floating_point_bounds )
{
    Bounded_variable<float> param( 1.5f, 0.0f, 10.0f );

    ASSERT_FLOAT_EQ( param.get(), 1.5f );
    ASSERT_FLOAT_EQ( param.min_value(), 0.0f );
    ASSERT_FLOAT_EQ( param.max_value(), 10.0f );

    param = 5.7f;
    ASSERT_FLOAT_EQ( param.get(), 5.7f );

    param = -1.0f;  // Below minimum
    ASSERT_FLOAT_EQ( param.get(), 0.0f );

    param = 15.0f;  // Above maximum
    ASSERT_FLOAT_EQ( param.get(), 10.0f );
}

TEST( bounded_variable, double_bounds )
{
    Bounded_variable<double> param( 2.5, 1.0, 5.0 );

    ASSERT_DOUBLE_EQ( param.get(), 2.5 );
    ASSERT_DOUBLE_EQ( param.min_value(), 1.0 );
    ASSERT_DOUBLE_EQ( param.max_value(), 5.0 );

    param = 3.14159;
    ASSERT_DOUBLE_EQ( param.get(), 3.14159 );

    param = 0.5;  // Below minimum
    ASSERT_DOUBLE_EQ( param.get(), 1.0 );

    param = 10.0;  // Above maximum
    ASSERT_DOUBLE_EQ( param.get(), 5.0 );
}

TEST( bounded_variable, observer_notification_on_valid_assignment )
{
    InSequence seq;
    Bounded_variable<int> param( 10, 100 );
    Mock_variable_observer observer( param );

    // Should receive notification when value changes within bounds
    EXPECT_CALL( observer, changed( 50 ) );
    param = 50;

    EXPECT_CALL( observer, changed( 75 ) );
    param = 75;
}

TEST( bounded_variable, observer_notification_on_clamped_assignment )
{
    Bounded_variable<int> param( 50, 10, 100 );  // Start with middle value
    Mock_variable_observer observer( param );

    // Should receive notification with clamped value when assigned out of bounds
    EXPECT_CALL( observer, changed( 10 ) );  // Clamped to minimum
    param = 5;

    EXPECT_CALL( observer, changed( 100 ) );  // Clamped to maximum
    param = 150;
}

TEST( bounded_variable, observer_notification_no_change )
{
    InSequence seq;
    Bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Should not receive notification when value doesn't change
    EXPECT_CALL( observer, changed ).Times( 0 );
    param = 50;  // Same value
}

TEST( bounded_variable, observer_notification_on_init )
{
    InSequence seq;
    Bounded_variable<int> param( 10, 100 );
    Mock_variable_observer observer( param );

    // observers are not supposed to be notified on variable init
    EXPECT_CALL( observer, changed ).Times( 0 );
    param.init( 50 );

    // But should be notified on subsequent changes
    EXPECT_CALL( observer, changed( 75 ) );
    param = 75;
}

TEST( bounded_variable, negative_bounds )
{
    Bounded_variable<int> param( -50, -100, -10 );

    ASSERT_EQ( param.get(), -50 );
    ASSERT_EQ( param.min_value(), -100 );
    ASSERT_EQ( param.max_value(), -10 );

    param = -75;
    ASSERT_EQ( param.get(), -75 );

    param = -150;  // Below minimum
    ASSERT_EQ( param.get(), -100 );

    param = 0;  // Above maximum
    ASSERT_EQ( param.get(), -10 );
}

TEST( bounded_variable, zero_range )
{
    Bounded_variable<int> param( 42, 42 );  // Min equals max

    ASSERT_EQ( param.get(), 42 );
    ASSERT_EQ( param.min_value(), 42 );
    ASSERT_EQ( param.max_value(), 42 );

    // Any assignment should result in the fixed value
    param = 100;
    ASSERT_EQ( param.get(), 42 );

    param = 0;
    ASSERT_EQ( param.get(), 42 );
}

TEST( bounded_variable, conversion_operator )
{
    Bounded_variable<int> param( 50, 10, 100 );

    // Test implicit conversion
    int value = param;
    ASSERT_EQ( value, 50 );

    // Test in arithmetic operations
    int result = param + 25;
    ASSERT_EQ( result, 75 );

    // Test in comparisons
    ASSERT_TRUE( param > 40 );
    ASSERT_TRUE( param < 60 );
    ASSERT_TRUE( param == 50 );
}

TEST( bounded_variable, unsigned_types )
{
    Bounded_variable<unsigned int> param( 50u, 10u, 100u );

    ASSERT_EQ( param.get(), 50u );
    ASSERT_EQ( param.min_value(), 10u );
    ASSERT_EQ( param.max_value(), 100u );

    param = 75u;
    ASSERT_EQ( param.get(), 75u );

    // Test with value that would be negative if signed
    param = 0u;  // Below minimum
    ASSERT_EQ( param.get(), 10u );

    param = 200u;  // Above maximum
    ASSERT_EQ( param.get(), 100u );
}

TEST( bounded_variable, char_type )
{
    Bounded_variable<char> param( 'M', 'A', 'Z' );

    ASSERT_EQ( param.get(), 'M' );
    ASSERT_EQ( param.min_value(), 'A' );
    ASSERT_EQ( param.max_value(), 'Z' );

    param = 'X';
    ASSERT_EQ( param.get(), 'X' );

    param = '5';  // Below 'A'
    ASSERT_EQ( param.get(), 'A' );

    param = 'z';  // Above 'Z'
    ASSERT_EQ( param.get(), 'Z' );
}

TEST( bounded_variable, nonfinite_updates_are_rejected )
{
    Bounded_variable<float> param( 5.0f, 0.0f, 10.0f );
    Mock_variable_observer observer( param );

    EXPECT_CALL( observer, changed ).Times( 0 );

    param.init( std::numeric_limits<float>::quiet_NaN() );
    param = std::numeric_limits<float>::infinity();

    EXPECT_EQ( param.get(), 5.0f );
}
