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

using namespace xe;
using namespace xe::cpu;
using namespace xe::cpu::hir;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

// =============================================================================
// FPCR isolation: VMX vector float ops must not leak their FPCR state into
// subsequent scalar FP operations.
//
// The bug: without scoped FPCR save/restore, a VMX op sets FPCR to
// round-to-nearest + flush-to-zero, and that state persists into the next
// scalar FP op — overriding whatever rounding mode the scalar path expects.
// =============================================================================

TEST_CASE("VMX_FPCR_DOES_NOT_LEAK_INTO_SCALAR", "[backend]") {
  // Strategy:
  //   1. SET_ROUNDING_MODE to toward-positive-infinity (mode 2)
  //   2. Do a VMX vector float add (internally sets FPCR to RN + FZ)
  //   3. Do a scalar float add of 1.0 + 2^-24
  //   4. If FPCR leaked, the scalar add uses round-to-nearest → result = 1.0
  //      If FPCR was properly restored, it uses toward-+inf → result > 1.0
  TestFunction test([](HIRBuilder& b) {
    // Set scalar rounding to toward +infinity.
    b.SetRoundingMode(b.LoadConstantInt32(2));

    // VMX vector float add — this touches FPCR internally.
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));

    // Now do a scalar float add. If FPCR leaked, this will round-to-nearest.
    auto a = b.Convert(LoadFPR(b, 6), FLOAT32_TYPE);
    auto c = b.Convert(LoadFPR(b, 7), FLOAT32_TYPE);
    auto sum = b.Add(a, c);
    StoreFPR(b, 3, b.Convert(sum, FLOAT64_TYPE));

    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        // Vector inputs — just normal values, we don't care about the result.
        ctx->v[4] = vec128f(1.0f, 2.0f, 3.0f, 4.0f);
        ctx->v[5] = vec128f(5.0f, 6.0f, 7.0f, 8.0f);
        // Scalar inputs: 1.0 + 2^-24 — rounds differently under different
        // modes.
        ctx->f[6] = 1.0;
        ctx->f[7] = std::ldexp(1.0, -24);
      },
      [&test](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        // Under toward-+infinity, 1.0f + 2^-24 rounds UP to nextafter(1.0f).
        // Under round-to-nearest, it rounds to 1.0f (ties to even).
        float expected = std::nextafterf(1.0f, 2.0f);
        REQUIRE(result == expected);
        // Reset rounding mode for subsequent tests.
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 0);
      });
}

TEST_CASE("VMX_FPCR_DOES_NOT_LEAK_INTO_SCALAR_MULTIPLE_OPS", "[backend]") {
  // Same idea but with two consecutive VMX vector float adds before the
  // scalar op, to verify FPCR is restored after each one.
  TestFunction test([](HIRBuilder& b) {
    b.SetRoundingMode(b.LoadConstantInt32(2));  // toward +inf

    // Two VMX vector float adds back to back.
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    StoreVR(b, 6, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 3), FLOAT32_TYPE));

    // Scalar add — must still use toward-+inf.
    auto a = b.Convert(LoadFPR(b, 6), FLOAT32_TYPE);
    auto c = b.Convert(LoadFPR(b, 7), FLOAT32_TYPE);
    auto sum = b.Add(a, c);
    StoreFPR(b, 3, b.Convert(sum, FLOAT64_TYPE));

    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->v[4] = vec128f(1.0f, 2.0f, 3.0f, 4.0f);
        ctx->v[5] = vec128f(5.0f, 6.0f, 7.0f, 8.0f);
        ctx->f[6] = 1.0;
        ctx->f[7] = std::ldexp(1.0, -24);
      },
      [&test](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        float expected = std::nextafterf(1.0f, 2.0f);
        REQUIRE(result == expected);
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 0);
      });
}
