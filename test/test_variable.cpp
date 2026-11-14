// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "test_common.h"
#include "mock_variable_observer.h"

#include "soma/variables/variable.h"
#include "soma/variables/bounded_variable.h"
#include "soma/variables/overridable_variable.h"
#include "soma/variables/overridable_bounded_variable.h"
#include "soma/variables/variable_observer_handler.h"

#include <cstring>
#include <type_traits>

using namespace SOMA;

static_assert( !std::is_copy_constructible_v<Variable<int>> );
static_assert( std::is_copy_assignable_v<Variable<int>> );
static_assert( !std::is_copy_constructible_v<Bounded_variable<int>> );
static_assert( std::is_copy_assignable_v<Bounded_variable<int>> );
static_assert( !std::is_copy_constructible_v<Overridable_variable<int>> );
static_assert( std::is_copy_assignable_v<Overridable_variable<int>> );
static_assert( !std::is_copy_constructible_v<Overridable_bounded_variable<int>> );
static_assert( std::is_copy_assignable_v<Overridable_bounded_variable<int>> );

struct Test_type_t
{
    int64_t a;
    int64_t b;

    bool operator==( const Test_type_t& rhs ) const
    {
        bool rv_equal = true;

        rv_equal = rv_equal && ( a == rhs.a );
        rv_equal = rv_equal && ( b == rhs.b );

        return rv_equal;
    }
};

Text_output& operator<<( Text_output& str,
                         const Test_type_t value )
{
    // intentionally empty

    return str;
}

struct Padded_test_type_t
{
    uint8_t status;
    uint8_t ignored;
    uint32_t count;

    bool operator==( const Padded_test_type_t& rhs ) const
    {
        return ( status == rhs.status ) && ( count == rhs.count );
    }
};

static_assert( !std::has_unique_object_representations_v<Padded_test_type_t> );

Text_output& operator<<( Text_output& str,
                         const Padded_test_type_t value )
{
    // intentionally empty

    return str;
}

enum class Variable_mode
{
    idle,
    active
};

static_assert( Is_inequality_comparable_t<Variable_mode>::value );

Text_output& operator<<( Text_output& str,
                         Variable_mode value )
{
    // intentionally empty

    return str;
}

TEST( variable, set )
{
    InSequence seq;
    Variable<int> P_tested( 0 );

    ASSERT_EQ( P_tested, 0 );

    P_tested.init( 12 );

    ASSERT_EQ( P_tested, 12 );

    P_tested = 3;

    ASSERT_EQ( P_tested, 3 );
}

TEST( variable, assignment_copies_only_value )
{
    const Variable<int> source( 17 );
    Variable<int> target( 0 );

    target = source;

    EXPECT_EQ( target.get(), 17 );
}

TEST( variable, set_struct )
{
    Test_type_t current_value;

    InSequence seq;
    Variable<Test_type_t> P_tested( Test_type_t {} );

    Test_type_t init_value = { 4, 2 };

    P_tested.init( init_value );
    current_value = P_tested;

    ASSERT_MEM_EQ( &current_value, &init_value, sizeof( Test_type_t ) );

    Test_type_t new_value = { 66, -666 };

    P_tested = new_value;
    current_value = P_tested;

    ASSERT_MEM_EQ( &current_value, &new_value, sizeof( Test_type_t ) );
}

TEST( variable, observer_notification )
{
    InSequence seq;
    Variable<int> P_tested( 0 );
    Mock_variable_observer observer( P_tested );

    // observers are not supposed to be notified on variable init
    EXPECT_CALL( observer, changed ).Times( 0 );
    P_tested.init( 1 );

    EXPECT_CALL( observer, changed( 2 ) );
    P_tested = 2;

    EXPECT_CALL( observer, changed( 3 ) );
    P_tested = 3;
}

TEST( variable, observer_notification_struct )
{
    InSequence seq;
    Variable<Test_type_t> P_tested( Test_type_t {} );
    Mock_variable_observer observer( P_tested );
    Test_type_t init_value = { 4, 2 };
    Test_type_t new_value = { 66, -666 };
    Test_type_t new_value2 = { -1, 42 };

    // observers are not supposed to be notified on variable init
    EXPECT_CALL( observer, changed ).Times( 0 );
    P_tested.init( init_value );

    EXPECT_CALL( observer, changed( new_value ) );
    P_tested = new_value;

    EXPECT_CALL( observer, changed( new_value2 ) );
    P_tested = new_value2;
}

TEST( variable, struct_with_padding_uses_value_equality )
{
    Padded_test_type_t initial;
    Padded_test_type_t equal_value;
    std::memset( &initial, 0x00, sizeof( initial ) );
    std::memset( &equal_value, 0xFF, sizeof( equal_value ) );
    initial.status = equal_value.status = 3;
    initial.count = equal_value.count = 42;
    // Distinct ignored data makes byte comparison report a change on every platform.
    ASSERT_NE( initial.ignored, equal_value.ignored );

    Variable<Padded_test_type_t> variable( initial );
    Mock_variable_observer observer( variable );

    EXPECT_CALL( observer, changed( _ ) ).Times( 0 );
    variable = equal_value;
    ::testing::Mock::VerifyAndClearExpectations( &observer );

    EXPECT_CALL( observer, changed( _ ) ).Times( 1 );
    equal_value.count = 43;
    variable = equal_value;
}

TEST( variable, enum_values_use_builtin_comparison )
{
    Variable<Variable_mode> variable( Variable_mode::idle );
    Mock_variable_observer observer( variable );

    EXPECT_CALL( observer, changed( _ ) ).Times( 0 );
    variable = Variable_mode::idle;
    ::testing::Mock::VerifyAndClearExpectations( &observer );

    EXPECT_CALL( observer, changed( Variable_mode::active ) ).Times( 1 );
    variable = Variable_mode::active;
}

TEST( variable, multiple_observers )
{
    InSequence seq;
    Variable<int> P_tested( 0 );
    Mock_variable_observer observer1( P_tested );
    Mock_variable_observer observer2( P_tested );
    Mock_variable_observer observer3( P_tested );

    EXPECT_CALL( observer1, changed( 1 ) );
    EXPECT_CALL( observer2, changed( 1 ) );
    EXPECT_CALL( observer3, changed( 1 ) );
    P_tested = 1;

    EXPECT_CALL( observer1, changed( 2 ) );
    EXPECT_CALL( observer2, changed( 2 ) );
    EXPECT_CALL( observer3, changed( 2 ) );
    P_tested = 2;

    EXPECT_CALL( observer1, changed( 3 ) );
    EXPECT_CALL( observer2, changed( 3 ) );
    EXPECT_CALL( observer3, changed( 3 ) );
    P_tested = 3;
}

TEST( variable, override )
{
    InSequence seq;
    Variable<int> P_tested( 0 );
    Mock_variable_observer observer( P_tested );

    EXPECT_CALL( observer, changed( 1 ) );
    P_tested = 1;

    EXPECT_CALL( observer, changed ).Times( 0 );
    P_tested.set_override( 2 );

    ASSERT_EQ( P_tested, 1 );
}
