// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"
#include "mock_variable_observer.h"

#include "soma/variables/overridable_bounded_variable.h"
#include "soma/variables/variable_observer_handler.h"

#include <limits>

using namespace SOMA;

TEST( overridable_bounded_variable, constructor_min_max_only )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    // Constructor with min/max should initialize with min value
    ASSERT_EQ( param.get(), 10 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
    ASSERT_FALSE( param.is_overridden() );
    ASSERT_TRUE( param.is_overridable() );
}

TEST( overridable_bounded_variable, constructor_with_value_within_bounds )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    ASSERT_EQ( param.get(), 50 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
    ASSERT_FALSE( param.is_overridden() );
    ASSERT_TRUE( param.is_overridable() );
}

TEST( overridable_bounded_variable, constructor_with_value_at_min_bound )
{
    Overridable_bounded_variable<int> param( 10, 10, 100 );

    ASSERT_EQ( param.get(), 10 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
}

TEST( overridable_bounded_variable, constructor_with_value_at_max_bound )
{
    Overridable_bounded_variable<int> param( 100, 10, 100 );

    ASSERT_EQ( param.get(), 100 );
    ASSERT_EQ( param.min_value(), 10 );
    ASSERT_EQ( param.max_value(), 100 );
}

TEST( overridable_bounded_variable, init_within_bounds )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param.init( 50 );

    ASSERT_EQ( param.get(), 50 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, init_at_min_bound )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param.init( 10 );

    ASSERT_EQ( param.get(), 10 );
}

TEST( overridable_bounded_variable, init_at_max_bound )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param.init( 100 );

    ASSERT_EQ( param.get(), 100 );
}

TEST( overridable_bounded_variable, init_below_min_bound_clamped )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param.init( 5 );

    // Value should be clamped to minimum
    ASSERT_EQ( param.get(), 10 );
}

TEST( overridable_bounded_variable, init_above_max_bound_clamped )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param.init( 150 );

    // Value should be clamped to maximum
    ASSERT_EQ( param.get(), 100 );
}

TEST( overridable_bounded_variable, assignment_within_bounds )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param = 75;

    ASSERT_EQ( param.get(), 75 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, assignment_at_min_bound )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param = 10;

    ASSERT_EQ( param.get(), 10 );
}

TEST( overridable_bounded_variable, assignment_at_max_bound )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param = 100;

    ASSERT_EQ( param.get(), 100 );
}

TEST( overridable_bounded_variable, assignment_below_min_bound_clamped )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param = -5;

    // Value should be clamped to minimum
    ASSERT_EQ( param.get(), 10 );
}

TEST( overridable_bounded_variable, assignment_above_max_bound_clamped )
{
    Overridable_bounded_variable<int> param( 10, 100 );

    param = 200;

    // Value should be clamped to maximum
    ASSERT_EQ( param.get(), 100 );
}

TEST( overridable_bounded_variable, assignment_from_another_variable )
{
    Overridable_bounded_variable<int> param1( 50, 10, 100 );
    Overridable_bounded_variable<int> param2( 10, 100 );

    param2 = param1;

    ASSERT_EQ( param2.get(), 50 );
}

TEST( overridable_bounded_variable, assignment_from_another_variable_clamped )
{
    Overridable_bounded_variable<int> param1( 80, 70, 90 );  // Different bounds
    Overridable_bounded_variable<int> param2( 10, 60 );      // Max bound is 60

    param2 = param1;  // 80 should be clamped to 60

    ASSERT_EQ( param2.get(), 60 );
}

TEST( overridable_bounded_variable, self_assignment_preserves_saved_value )
{
    Overridable_bounded_variable<int> variable( 3, 0, 10 );

    ASSERT_TRUE( variable.set_override( 9 ) );
    variable = variable;
    variable.clear_override();

    EXPECT_EQ( variable.get(), 3 );
}

TEST( overridable_bounded_variable, override_within_bounds )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    ASSERT_TRUE( param.set_override( 75 ) );

    ASSERT_EQ( param.get(), 75 );
    ASSERT_TRUE( param.is_overridden() );
}

TEST( overridable_bounded_variable, string_override_uses_bounds )
{
    Overridable_bounded_variable<int> variable( 50, 10, 100 );

    EXPECT_TRUE( variable.set_override( String_view( "150" ) ) );
    EXPECT_EQ( variable.get(), 100 );
    EXPECT_TRUE( variable.is_overridden() );

    variable.clear_override();
    EXPECT_EQ( variable.get(), 50 );

    EXPECT_FALSE( variable.set_override( String_view( "4294967296" ) ) );
    EXPECT_EQ( variable.get(), 50 );
    EXPECT_FALSE( variable.is_overridden() );
}

TEST( overridable_bounded_variable, override_at_min_bound )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    ASSERT_TRUE( param.set_override( 10 ) );

    ASSERT_EQ( param.get(), 10 );
    ASSERT_TRUE( param.is_overridden() );
}

TEST( overridable_bounded_variable, override_at_max_bound )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    ASSERT_TRUE( param.set_override( 100 ) );

    ASSERT_EQ( param.get(), 100 );
    ASSERT_TRUE( param.is_overridden() );
}

TEST( overridable_bounded_variable, override_below_min_bound_clamped )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    ASSERT_TRUE( param.set_override( 5 ) );

    // Value should be clamped to minimum
    ASSERT_EQ( param.get(), 10 );
    ASSERT_TRUE( param.is_overridden() );
}

TEST( overridable_bounded_variable, override_above_max_bound_clamped )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    ASSERT_TRUE( param.set_override( 150 ) );

    // Value should be clamped to maximum
    ASSERT_EQ( param.get(), 100 );
    ASSERT_TRUE( param.is_overridden() );
}

TEST( overridable_bounded_variable, override_then_clear )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    // Override with a different value
    ASSERT_TRUE( param.set_override( 75 ) );
    ASSERT_EQ( param.get(), 75 );
    ASSERT_TRUE( param.is_overridden() );

    // Clear override - should return to original value
    param.clear_override();
    ASSERT_EQ( param.get(), 50 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, override_then_clear_clamped_value )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    // Override with out-of-bounds value (gets clamped)
    ASSERT_TRUE( param.set_override( 150 ) );
    ASSERT_EQ( param.get(), 100 );  // Clamped
    ASSERT_TRUE( param.is_overridden() );

    // Clear override - should return to original value
    param.clear_override();
    ASSERT_EQ( param.get(), 50 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, assignment_while_overridden )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    // Override the value
    ASSERT_TRUE( param.set_override( 75 ) );
    ASSERT_EQ( param.get(), 75 );

    // Assign new value while overridden (should be saved but not applied)
    param = 60;
    ASSERT_EQ( param.get(), 75 );  // Still the override value
    ASSERT_TRUE( param.is_overridden() );

    // Clear override - should get the new assigned value
    param.clear_override();
    ASSERT_EQ( param.get(), 60 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, assignment_while_overridden_clamped )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    // Override the value
    ASSERT_TRUE( param.set_override( 75 ) );
    ASSERT_EQ( param.get(), 75 );

    // Assign out-of-bounds value while overridden
    param = 150;  // Above max
    ASSERT_EQ( param.get(), 75 );  // Still the override value
    ASSERT_TRUE( param.is_overridden() );

    // Clear override - should get the clamped assigned value
    param.clear_override();
    ASSERT_EQ( param.get(), 100 );  // Clamped to max
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, multiple_overrides )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

    // First override
    ASSERT_TRUE( param.set_override( 75 ) );
    ASSERT_EQ( param.get(), 75 );
    ASSERT_TRUE( param.is_overridden() );

    // Second override (should replace first)
    ASSERT_TRUE( param.set_override( 85 ) );
    ASSERT_EQ( param.get(), 85 );
    ASSERT_TRUE( param.is_overridden() );

    // Clear override - should return to original value (not first override)
    param.clear_override();
    ASSERT_EQ( param.get(), 50 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, floating_point_bounds )
{
    Overridable_bounded_variable<float> param( 1.5f, 0.0f, 10.0f );

    ASSERT_FLOAT_EQ( param.get(), 1.5f );
    ASSERT_FLOAT_EQ( param.min_value(), 0.0f );
    ASSERT_FLOAT_EQ( param.max_value(), 10.0f );

    // Test normal assignment
    param = 5.7f;
    ASSERT_FLOAT_EQ( param.get(), 5.7f );

    // Test override
    ASSERT_TRUE( param.set_override( 8.2f ) );
    ASSERT_FLOAT_EQ( param.get(), 8.2f );
    ASSERT_TRUE( param.is_overridden() );

    // Test clamped override
    ASSERT_TRUE( param.set_override( -1.0f ) );  // Below minimum
    ASSERT_FLOAT_EQ( param.get(), 0.0f );

    ASSERT_TRUE( param.set_override( 15.0f ) );  // Above maximum
    ASSERT_FLOAT_EQ( param.get(), 10.0f );

    // Clear override
    param.clear_override();
    ASSERT_FLOAT_EQ( param.get(), 5.7f );  // Back to assigned value
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, nonfinite_override_is_rejected )
{
    Overridable_bounded_variable<float> variable( 5.0f, 0.0f, 10.0f );

    EXPECT_FALSE( variable.set_override( std::numeric_limits<float>::quiet_NaN() ) );
    EXPECT_FALSE( variable.set_override( String_view( "nan" ) ) );
    EXPECT_FALSE( variable.is_overridden() );
    EXPECT_EQ( variable.get(), 5.0f );
}

TEST( overridable_bounded_variable, nonfinite_owner_updates_are_rejected )
{
    Overridable_bounded_variable<float> param( 5.0f, 0.0f, 10.0f );
    Mock_variable_observer observer( param );

    EXPECT_CALL( observer, changed ).Times( 0 );

    param.init( std::numeric_limits<float>::quiet_NaN() );
    param = std::numeric_limits<float>::infinity();

    EXPECT_EQ( param.get(), 5.0f );
    EXPECT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, observer_notification_on_assignment )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Should receive notification when value changes within bounds
    EXPECT_CALL( observer, changed( 75 ) );
    param = 75;

    EXPECT_CALL( observer, changed( 60 ) );
    param = 60;
}

TEST( overridable_bounded_variable, observer_notification_on_clamped_assignment )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Should receive notification with clamped value when assigned out of bounds
    EXPECT_CALL( observer, changed( 10 ) );  // Clamped to minimum
    param = 5;

    EXPECT_CALL( observer, changed( 100 ) );  // Clamped to maximum
    param = 150;
}

TEST( overridable_bounded_variable, observer_notification_on_override )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Should receive notification when override is set
    EXPECT_CALL( observer, changed( 75 ) );
    param.set_override( 75 );

    // Should receive notification when override is changed
    EXPECT_CALL( observer, changed( 85 ) );
    param.set_override( 85 );
}

TEST( overridable_bounded_variable, observer_notification_on_clamped_override )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Should receive notification with clamped value when override is out of bounds
    EXPECT_CALL( observer, changed( 10 ) );  // Clamped to minimum
    param.set_override( 5 );

    EXPECT_CALL( observer, changed( 100 ) );  // Clamped to maximum
    param.set_override( 150 );
}

TEST( overridable_bounded_variable, observer_notification_on_clear_override )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Set override
    EXPECT_CALL( observer, changed( 75 ) );
    param.set_override( 75 );

    // Clear override - should notify with original value
    EXPECT_CALL( observer, changed( 50 ) );
    param.clear_override();
}

TEST( overridable_bounded_variable, observer_notification_no_change )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );
    Mock_variable_observer observer( param );

    // Should not receive notification when value doesn't change
    EXPECT_CALL( observer, changed ).Times( 0 );
    param = 50;  // Same value

    // Should not receive notification when override is same as current value
    EXPECT_CALL( observer, changed ).Times( 0 );
    param.set_override( 50 );
}

TEST( overridable_bounded_variable, observer_notification_on_init )
{
    Overridable_bounded_variable<int> param( 10, 100 );
    Mock_variable_observer observer( param );

    // observers are not supposed to be notified on variable init
    EXPECT_CALL( observer, changed ).Times( 0 );
    param.init( 50 );

    // But should be notified on subsequent changes
    EXPECT_CALL( observer, changed( 75 ) );
    param = 75;
}

TEST( overridable_bounded_variable, negative_bounds )
{
    Overridable_bounded_variable<int> param( -50, -100, -10 );

    ASSERT_EQ( param.get(), -50 );
    ASSERT_EQ( param.min_value(), -100 );
    ASSERT_EQ( param.max_value(), -10 );

    param = -75;
    ASSERT_EQ( param.get(), -75 );

    param = -150;  // Below minimum
    ASSERT_EQ( param.get(), -100 );

    param = 0;  // Above maximum
    ASSERT_EQ( param.get(), -10 );

    // Test override with negative bounds
    ASSERT_TRUE( param.set_override( -80 ) );
    ASSERT_EQ( param.get(), -80 );

    ASSERT_TRUE( param.set_override( -200 ) );  // Below minimum
    ASSERT_EQ( param.get(), -100 );

    ASSERT_TRUE( param.set_override( 10 ) );  // Above maximum
    ASSERT_EQ( param.get(), -10 );
}

TEST( overridable_bounded_variable, zero_range )
{
    Overridable_bounded_variable<int> param( 42, 42 );  // Min equals max

    ASSERT_EQ( param.get(), 42 );
    ASSERT_EQ( param.min_value(), 42 );
    ASSERT_EQ( param.max_value(), 42 );

    // Any assignment should result in the fixed value
    param = 100;
    ASSERT_EQ( param.get(), 42 );

    param = 0;
    ASSERT_EQ( param.get(), 42 );

    // Override should also be clamped to the fixed value
    ASSERT_TRUE( param.set_override( 100 ) );
    ASSERT_EQ( param.get(), 42 );
    ASSERT_TRUE( param.is_overridden() );

    param.clear_override();
    ASSERT_EQ( param.get(), 42 );
    ASSERT_FALSE( param.is_overridden() );
}

TEST( overridable_bounded_variable, conversion_operator )
{
    Overridable_bounded_variable<int> param( 50, 10, 100 );

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

    // Test with override
    param.set_override( 75 );
    int override_value = param;
    ASSERT_EQ( override_value, 75 );
}

TEST( overridable_bounded_variable, unsigned_types )
{
    Overridable_bounded_variable<unsigned int> param( 50u, 10u, 100u );

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

    // Test override
    ASSERT_TRUE( param.set_override( 85u ) );
    ASSERT_EQ( param.get(), 85u );
    ASSERT_TRUE( param.is_overridden() );

    param.clear_override();
    ASSERT_EQ( param.get(), 100u );  // Back to last assigned value
}

TEST( overridable_bounded_variable, char_type )
{
    Overridable_bounded_variable<char> param( 'M', 'A', 'Z' );

    ASSERT_EQ( param.get(), 'M' );
    ASSERT_EQ( param.min_value(), 'A' );
    ASSERT_EQ( param.max_value(), 'Z' );

    param = 'X';
    ASSERT_EQ( param.get(), 'X' );

    param = '5';  // Below 'A'
    ASSERT_EQ( param.get(), 'A' );

    param = 'z';  // Above 'Z'
    ASSERT_EQ( param.get(), 'Z' );

    // Test override
    ASSERT_TRUE( param.set_override( 'Q' ) );
    ASSERT_EQ( param.get(), 'Q' );

    ASSERT_TRUE( param.set_override( '1' ) );  // Below 'A'
    ASSERT_EQ( param.get(), 'A' );

    param.clear_override();
    ASSERT_EQ( param.get(), 'Z' );  // Back to last assigned value
}
