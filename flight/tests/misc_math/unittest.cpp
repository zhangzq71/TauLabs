/**
 ******************************************************************************
 * @file       unittest.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup UnitTests
 * @{
 * @addtogroup UnitTests
 * @{
 * @brief Unit test
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * NOTE: This program uses the Google Test infrastructure to drive the unit test
 *
 * Main site for Google Test: http://code.google.com/p/googletest/
 * Documentation and examples: http://code.google.com/p/googletest/wiki/Documentation
 */

#include "gtest/gtest.h"

#include <stdio.h>		/* printf */
#include <stdlib.h>		/* abort */
#include <string.h>		/* memset */
#include <stdint.h>		/* uint*_t */

extern "C" {

#include "misc_math.h"		/* API for misc_math functions */

}

#include <math.h>		/* fabs() */

// To use a test fixture, derive a class from testing::Test.
class MiscMath : public testing::Test {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

// Test fixture for bound_min_max()
class BoundMinMax : public MiscMath {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(BoundMinMax, ValBelowZeroRange) {
  // Test lower bounding when min = max with (val < min)
  EXPECT_EQ(-1.0f, bound_min_max(-10.0f, -1.0f, -1.0f));
  EXPECT_EQ(0.0f, bound_min_max(-10.0f, 0.0f, 0.0f));
  EXPECT_EQ(1.0f, bound_min_max(-10.0f, 1.0f, 1.0f));
};

TEST_F(BoundMinMax, ValWithinZeroRange) {
  // Test bounding when min = max = val
  EXPECT_EQ(-1.0f, bound_min_max(-1.0f, -1.0f, -1.0f));
  EXPECT_EQ(0.0f, bound_min_max(0.0f, 0.0f, 0.0f));
  EXPECT_EQ(1.0f, bound_min_max(1.0f, 1.0f, 1.0f));
};

TEST_F(BoundMinMax, ValAboveZeroRange) {
  // Test upper bounding when min = max with (val > max)
  EXPECT_EQ(-1.0f, bound_min_max(10.0f, -1.0f, -1.0f));
  EXPECT_EQ(0.0f, bound_min_max(10.0f, 0.0f, 0.0f));
  EXPECT_EQ(1.0f, bound_min_max(10.0f, 1.0f, 1.0f));
}

TEST_F(BoundMinMax, PositiveMinMax) {
  float min = 1.0f;
  float max = 10.0f;

  // Below Lower Bound
  EXPECT_EQ(min, bound_min_max(min - 1.0f, min, max));
  // At Lower Bound
  EXPECT_EQ(min, bound_min_max(min, min, max));
  // In Bounds
  EXPECT_EQ(2.0f, bound_min_max(2.0f, min, max));
  // At Upper Bound
  EXPECT_EQ(max, bound_min_max(max, min, max));
  // Above Upper Bound
  EXPECT_EQ(max, bound_min_max(max + 1.0f, min, max));
}

TEST_F(BoundMinMax, NegativeMinMax) {
  float min = -10.0f;
  float max = -1.0f;

  // Below Lower Bound
  EXPECT_EQ(min, bound_min_max(min - 1.0f, min, max));
  // At Lower Bound
  EXPECT_EQ(min, bound_min_max(min, min, max));
  // In Bounds
  EXPECT_EQ(-2.0f, bound_min_max(-2.0f, min, max));
  // At Upper Bound
  EXPECT_EQ(max, bound_min_max(max, min, max));
  // Above Upper Bound
  EXPECT_EQ(max, bound_min_max(max + 1.0f, min, max));
}

TEST_F(BoundMinMax, StraddleZeroMinMax) {
  float min = -10.0f;
  float max = 10.0f;

  // Below Lower Bound
  EXPECT_EQ(min, bound_min_max(min - 1.0f, min, max));
  // At Lower Bound
  EXPECT_EQ(min, bound_min_max(min, min, max));
  // In Bounds
  EXPECT_EQ(0.0f, bound_min_max(0.0f, min, max));
  // At Upper Bound
  EXPECT_EQ(max, bound_min_max(max, min, max));
  // Above Upper Bound
  EXPECT_EQ(max, bound_min_max(max + 1.0f, min, max));
}

// Test fixture for bound_sym()
class BoundSym : public MiscMath {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(BoundSym, ZeroRange) {
  float range = 0.0f;

  // Below Lower Bound
  EXPECT_EQ(-range, bound_sym(-range - 1.0f, range));
  // At Lower Bound
  EXPECT_EQ(-range, bound_sym(-range, range));
  // In Bounds
  EXPECT_EQ(0.0f, bound_sym(0.0f, range));
  // At Upper Bound
  EXPECT_EQ(range, bound_sym(range, range));
  // Above Upper Bound
  EXPECT_EQ(range, bound_sym(range + 1.0f, range));
};

TEST_F(BoundSym, NonZeroRange) {
  float range = 10.0f;

  // Below Lower Bound
  EXPECT_EQ(-range, bound_sym(-range - 1.0f, range));
  // At Lower Bound
  EXPECT_EQ(-range, bound_sym(-range, range));
  // In Bounds
  EXPECT_EQ(0.0f, bound_sym(0.0f, range));
  // At Upper Bound
  EXPECT_EQ(range, bound_sym(range, range));
  // Above Upper Bound
  EXPECT_EQ(range, bound_sym(range + 1.0f, range));
};

// Test fixture for circular_modulus_deg()
class CircularModulusDeg : public MiscMath {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(CircularModulusDeg, NullError) {
  float error = 0.0f;
  EXPECT_EQ(-error, circular_modulus_deg(error - 3600000));
  EXPECT_EQ(-error, circular_modulus_deg(error - 1080));
  EXPECT_EQ(-error, circular_modulus_deg(error - 720));
  EXPECT_EQ(-error, circular_modulus_deg(error - 360));
  EXPECT_EQ(-error, circular_modulus_deg(error));
  EXPECT_EQ(-error, circular_modulus_deg(error + 360));
  EXPECT_EQ(-error, circular_modulus_deg(error + 720));
  EXPECT_EQ(-error, circular_modulus_deg(error + 1080));
  EXPECT_EQ(-error, circular_modulus_deg(error + 3600000));
};

TEST_F(CircularModulusDeg, MaxPosError) {
  // Use fabs() for +/-180.0 to accept either -180.0 or +180.0 as valid and correct
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f - 3600000)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f - 1080)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f - 720)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f - 360)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f + 360)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f + 720)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f + 1080)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(180.0f + 3600000)));
};

TEST_F(CircularModulusDeg, MaxNegError) {
  // Use fabs() for +/-180.0 to accept either -180.0 or +180.0 as valid and correct
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f - 3600000)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f - 1080)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f - 720)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f - 360)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f + 360)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f + 720)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f + 1080)));
  EXPECT_EQ(180.0f, fabs(circular_modulus_deg(-180.0f + 3600000)));
};

TEST_F(CircularModulusDeg, SweepError) {
  float eps = 0.0001f;

  for (float error = -179.9f; error < 179.9f; error += 0.001f) {
    ASSERT_NEAR(error, circular_modulus_deg(error - 1080), eps);
    ASSERT_NEAR(error, circular_modulus_deg(error - 720), eps);
    ASSERT_NEAR(error, circular_modulus_deg(error - 360), eps);
    ASSERT_NEAR(error, circular_modulus_deg(error), eps);
    ASSERT_NEAR(error, circular_modulus_deg(error + 360), eps);
    ASSERT_NEAR(error, circular_modulus_deg(error + 720), eps);
    ASSERT_NEAR(error, circular_modulus_deg(error + 1080), eps);
  }
};

// Test fixture for vector2_clip()
class Vector2Clip : public MiscMath {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(Vector2Clip, TestScale) {
  float eps = 0.000001f;

  // Choose a non-integer limit
  for (int i=0; i<3; i++) {
    // Change the limit on each interation. This tests limit < 1, limit = 1, and limit > 1.
    float limit = 0.5*i;

    float test_vec_null[2] = {0, 0};
    float test_vec_within[2] = {limit/2, limit/2};
    float test_vec_edge_numerically_stable[2] = {limit, 0};
    float test_vec_edge_numerically_unstable[2] =
        {(float) (sqrt(2)/2*limit), (float) (sqrt(2)/2*limit)};
    float test_vec_outside[2] = {limit, limit};

    // Test for 0 vector
    vector2_clip(test_vec_null, limit);
    ASSERT_NEAR(test_vec_null[0], 0, eps);
    ASSERT_NEAR(test_vec_null[1], 0, eps);

    // Test for vector within limits
    vector2_clip(test_vec_within, limit);
    EXPECT_EQ(test_vec_within[0], limit/2);
    EXPECT_EQ(test_vec_within[1], limit/2);

    // Test for vector numerically identically at limits
    vector2_clip(test_vec_edge_numerically_stable, limit);
    EXPECT_EQ(test_vec_edge_numerically_stable[0], limit);
    EXPECT_EQ(test_vec_edge_numerically_stable[1], 0.0f);

    // Test for vector identically at limits, but suffering from numerical imprecision
    vector2_clip(test_vec_edge_numerically_unstable, limit);
    ASSERT_NEAR(test_vec_edge_numerically_unstable[0], sqrt(2)/2*limit, eps);
    ASSERT_NEAR(test_vec_edge_numerically_unstable[1], sqrt(2)/2*limit, eps);

    // Test for vector outside limits
    vector2_clip(test_vec_outside, limit);
    ASSERT_NEAR(test_vec_outside[0], sqrt(2)/2*limit, eps);
    ASSERT_NEAR(test_vec_outside[1], sqrt(2)/2*limit, eps);
  }
};

// Test fixture for linear_interpolate()
class LinearInterpolate : public MiscMath {
protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(LinearInterpolate, ThrottleCurve1to1) {
  float const range_min = 0.0f;
  float const range_max = 1.0f;
  uint8_t const curve_numpts = 5;
  float const curve[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
  float eps = 0.000001f;

  // 21 points in range
  for (size_t i = 0; i <= 20; ++i) {
    float const input = i * 0.05f;
    EXPECT_NEAR(input, linear_interpolate(input, curve, curve_numpts, range_min, range_max), eps);
  }

  // 10 points below min range
  for (size_t i = 1; i <= 10; ++i) {
    float const input = range_min - i * 0.1f;
    EXPECT_NEAR(range_min, linear_interpolate(input, curve, curve_numpts, range_min, range_max), eps);
  }

  // 10 points above max range
  for (size_t i = 1; i <= 10; ++i) {
    float const input = range_max + i * 0.1f;
    EXPECT_NEAR(range_max, linear_interpolate(input, curve, curve_numpts, range_min, range_max), eps);
  }
};

TEST_F(LinearInterpolate, CollectiveCurve1to1) {
  float const range_min = -1.0f;
  float const range_max = 1.0f;
  uint8_t const curve_numpts = 5;
  float const curve[5] = { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f };
  float eps = 0.000001f;

  // 21 points in range
  for (size_t i = 0; i <= 20; ++i) {
    float const input = i * 0.1f - 1.0f;
    EXPECT_NEAR(input, linear_interpolate(input, curve, curve_numpts, range_min, range_max), eps);
  }

  // 10 points below min range
  for (size_t i = 1; i <= 10; ++i) {
    float const input = range_min - i * 0.1f;
    EXPECT_NEAR(range_min, linear_interpolate(input, curve, curve_numpts, range_min, range_max), eps);
  }

  // 10 points above max range
  for (size_t i = 1; i <= 10; ++i) {
    float const input = range_max + i * 0.1f;
    EXPECT_NEAR(range_max, linear_interpolate(input, curve, curve_numpts, range_min, range_max), eps);
  }
};
