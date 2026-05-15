/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/cpu/testing/util.h"

#include <cfloat>

using namespace xe;
using namespace xe::cpu;
using namespace xe::cpu::hir;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

TEST_CASE("VECTOR_DENORMFLUSH_F32", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorDenormFlush(LoadVR(b, 4)));
    b.Return();
  });
  test.Run([](PPCContext* ctx) { ctx->v[4] = vec128i(0x7fffff); },
           [](PPCContext* ctx) {
             auto result = ctx->v[3];
             REQUIRE(result == vec128f(0.0f));
           });
  test.Run([](PPCContext* ctx) { ctx->v[4] = vec128i(0x807fffff); },
           [](PPCContext* ctx) {
             auto result = ctx->v[3];
             REQUIRE(result == vec128f(-0.0f));
           });
  test.Run([](PPCContext* ctx) { ctx->v[4] = vec128f(FLT_MIN); },
           [](PPCContext* ctx) {
             auto result = ctx->v[3];
             REQUIRE(result == vec128f(FLT_MIN));
           });
  test.Run([](PPCContext* ctx) { ctx->v[4] = vec128f(-FLT_MIN); },
           [](PPCContext* ctx) {
             auto result = ctx->v[3];
             REQUIRE(result == vec128f(-FLT_MIN));
           });
}

// Test VECTOR_DENORMFLUSH with register pressure: input is kept
// alive past the denormflush, forcing dest and src into different
// physical registers. Verifies correct passthrough of normal values
// and preservation of the original input.
TEST_CASE("VECTOR_DENORMFLUSH_F32_REGPRESSURE", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    auto input = LoadVR(b, 4);
    StoreVR(b, 3, b.VectorDenormFlush(input));
    StoreVR(b, 5, input);  // keep input alive past denormflush
    b.Return();
  });
  // Mix of denormal and normal elements:
  //   [0] 0x007FFFFF = positive denormal  -> should flush to +0.0
  //   [1] 0x3F800000 = 1.0f (normal)      -> should pass through
  //   [2] 0x807FFFFF = negative denormal  -> should flush to -0.0
  //   [3] 0x40000000 = 2.0f (normal)      -> should pass through
  test.Run(
      [](PPCContext* ctx) {
        ctx->v[4] = vec128i(0x007FFFFF, 0x3F800000, 0x807FFFFF, 0x40000000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        REQUIRE(result ==
                vec128i(0x00000000, 0x3F800000, 0x80000000, 0x40000000));
        // Original input must also be preserved
        REQUIRE(ctx->v[5] ==
                vec128i(0x007FFFFF, 0x3F800000, 0x807FFFFF, 0x40000000));
      });
  // All normal values - with the bug, ALL lanes would be corrupted
  // since k1 would be all-zero and merge-masking preserves dest, not src1
  test.Run(
      [](PPCContext* ctx) {
        ctx->v[4] = vec128i(0x3F800000, 0x40000000, 0x40400000, 0x40800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        REQUIRE(result ==
                vec128i(0x3F800000, 0x40000000, 0x40400000, 0x40800000));
      });
}
