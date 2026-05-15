/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/cpu/testing/util.h"

#include <cmath>
#include <cstring>

using namespace xe;
using namespace xe::cpu;
using namespace xe::cpu::hir;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

// PPC rounding mode values (as passed to SET_ROUNDING_MODE, bits 0-1):
//   0 = Round to nearest (ties to even)
//   1 = Round toward zero (truncate)
//   2 = Round toward +infinity (ceiling)
//   3 = Round toward -infinity (floor)

// Test that SET_ROUNDING_MODE actually changes the FPU rounding behavior.
// We add 1.0f + 2^-24 in f32. In round-to-nearest the result is 1.0f
// (the half-ULP bit rounds to even). In round-toward-positive-infinity
// the result is nextafterf(1.0f, 2.0f) = 1.0f + 2^-23.
TEST_CASE("SET_ROUNDING_MODE_TOWARD_POS_INF", "[instr]") {
  // Round toward +infinity (mode 2), then add.
  TestFunction test([](HIRBuilder& b) {
    b.SetRoundingMode(b.LoadConstantInt32(2));  // toward +inf
    auto a = b.Convert(LoadFPR(b, 4), FLOAT32_TYPE);
    auto c = b.Convert(LoadFPR(b, 5), FLOAT32_TYPE);
    auto sum = b.Add(a, c);
    StoreFPR(b, 3, b.Convert(sum, FLOAT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->f[4] = 1.0;
        // 2^-24 = 5.960464477539063e-08
        ctx->f[5] = std::ldexp(1.0, -24);
      },
      [&test](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        // With round-toward-positive-infinity, 1.0f + 2^-24 should round
        // up to the next representable float above 1.0f.
        float expected = std::nextafterf(1.0f, 2.0f);
        REQUIRE(result == expected);
        // Reset to nearest so subsequent tests aren't affected.
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 0);
      });
}

TEST_CASE("SET_ROUNDING_MODE_TOWARD_ZERO", "[instr]") {
  // Round toward zero (mode 1), then add a positive value.
  // For positive results, toward-zero == toward-negative-infinity (truncate).
  TestFunction test([](HIRBuilder& b) {
    b.SetRoundingMode(b.LoadConstantInt32(1));  // toward zero
    auto a = b.Convert(LoadFPR(b, 4), FLOAT32_TYPE);
    auto c = b.Convert(LoadFPR(b, 5), FLOAT32_TYPE);
    auto sum = b.Add(a, c);
    StoreFPR(b, 3, b.Convert(sum, FLOAT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->f[4] = 1.0;
        ctx->f[5] = std::ldexp(1.0, -24);
      },
      [&test](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        // With round-toward-zero, 1.0f + 2^-24 should truncate to 1.0f.
        REQUIRE(result == 1.0f);
        // Reset to nearest so subsequent tests aren't affected.
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 0);
      });
}

TEST_CASE("SET_ROUNDING_MODE_NEAREST", "[instr]") {
  // Round to nearest (mode 0) — default.
  TestFunction test([](HIRBuilder& b) {
    b.SetRoundingMode(b.LoadConstantInt32(0));  // nearest
    auto a = b.Convert(LoadFPR(b, 4), FLOAT32_TYPE);
    auto c = b.Convert(LoadFPR(b, 5), FLOAT32_TYPE);
    auto sum = b.Add(a, c);
    StoreFPR(b, 3, b.Convert(sum, FLOAT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->f[4] = 1.0;
        ctx->f[5] = std::ldexp(1.0, -24);
      },
      [](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        // With round-to-nearest, 1.0f + 2^-24 rounds to 1.0f (ties to even).
        REQUIRE(result == 1.0f);
      });
}
