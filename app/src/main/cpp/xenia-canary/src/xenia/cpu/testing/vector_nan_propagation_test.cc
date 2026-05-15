/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/cpu/testing/util.h"

using namespace xe;
using namespace xe::cpu;
using namespace xe::cpu::hir;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

// =============================================================================
// PPC NaN propagation rules for VMX vector float operations:
//
//   1. If src1 is NaN, result = src1 with bit 22 set (quieted).
//   2. Else if src2 is NaN, result = src2 with bit 22 set (quieted).
//   3. Else if the operation itself produces NaN (e.g., inf - inf),
//      result = PPC default QNaN = 0xFFC00000.
//
// These tests verify that behavior for VECTOR_ADD with FLOAT32_TYPE.
// =============================================================================

// src1 has QNaN in lane 0, src2 is normal → result lane 0 = src1's NaN quieted.
TEST_CASE("VECTOR_ADD_F32_NAN_SRC1_PROPAGATES", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        // QNaN with payload 0x1234 in lane 0; normal values elsewhere.
        ctx->v[4] = vec128i(0x7FC01234, 0x3F800000, 0x40000000, 0x40400000);
        ctx->v[5] = vec128i(0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        // Lane 0: src1 NaN wins, bit 22 set, payload preserved.
        REQUIRE((result.u32[0] & 0xFFC00000) == 0x7FC00000);
        REQUIRE((result.u32[0] & 0x003FFFFF) == 0x1234);
        // Lane 1: 1.0 + 1.0 = 2.0
        REQUIRE(result.u32[1] == 0x40000000);
      });
}

// src1 is normal, src2 has QNaN → result = src2's NaN quieted.
TEST_CASE("VECTOR_ADD_F32_NAN_SRC2_PROPAGATES", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->v[4] = vec128i(0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000);
        // QNaN with payload 0x5678 in lane 0.
        ctx->v[5] = vec128i(0x7FC05678, 0x3F800000, 0x3F800000, 0x3F800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        // Lane 0: src2 NaN wins (src1 was not NaN), payload preserved.
        REQUIRE((result.u32[0] & 0xFFC00000) == 0x7FC00000);
        REQUIRE((result.u32[0] & 0x003FFFFF) == 0x5678);
      });
}

// Both src1 and src2 have NaN in the same lane → src1 wins.
TEST_CASE("VECTOR_ADD_F32_NAN_BOTH_SRC1_WINS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        // src1: QNaN with payload 0xAAAA
        ctx->v[4] = vec128i(0x7FC0AAAA, 0x3F800000, 0x3F800000, 0x3F800000);
        // src2: QNaN with payload 0xBBBB
        ctx->v[5] = vec128i(0x7FC0BBBB, 0x3F800000, 0x3F800000, 0x3F800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        // src1's NaN wins when both are NaN.
        REQUIRE((result.u32[0] & 0xFFC00000) == 0x7FC00000);
        REQUIRE((result.u32[0] & 0x003FFFFF) == 0x0AAAA);
      });
}

// SNaN in src1 → result is src1 with bit 22 SET (quieted to QNaN).
TEST_CASE("VECTOR_ADD_F32_SNAN_QUIETED", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        // SNaN: exponent all 1s, bit 22 = 0, mantissa != 0.
        // 0x7F812345 = SNaN with payload.
        ctx->v[4] = vec128i(0x7F812345, 0x3F800000, 0x3F800000, 0x3F800000);
        ctx->v[5] = vec128i(0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        // SNaN quieted: bit 22 forced on, rest of payload preserved.
        REQUIRE(result.u32[0] == (0x7F812345 | 0x00400000));
      });
}

// Neither input is NaN, but operation produces NaN (inf + (-inf)).
// Result should be PPC default QNaN = 0xFFC00000.
TEST_CASE("VECTOR_ADD_F32_INF_MINUS_INF_DEFAULT_NAN", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        // +inf + (-inf) → NaN
        ctx->v[4] = vec128i(0x7F800000, 0x3F800000, 0x3F800000, 0x3F800000);
        ctx->v[5] = vec128i(0xFF800000, 0x3F800000, 0x3F800000, 0x3F800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        // PPC default QNaN (negative).
        REQUIRE(result.u32[0] == 0xFFC00000);
        // Other lanes: normal 1.0 + 1.0 = 2.0
        REQUIRE(result.u32[1] == 0x40000000);
      });
}

// NaN in different lanes — verify per-lane independence.
TEST_CASE("VECTOR_ADD_F32_NAN_PER_LANE", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreVR(b, 3, b.VectorAdd(LoadVR(b, 4), LoadVR(b, 5), FLOAT32_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        // Lane 0: normal + normal
        // Lane 1: NaN in src1
        // Lane 2: NaN in src2
        // Lane 3: inf + (-inf)
        ctx->v[4] = vec128i(0x3F800000, 0x7FC00001, 0x3F800000, 0x7F800000);
        ctx->v[5] = vec128i(0x3F800000, 0x3F800000, 0x7FC00002, 0xFF800000);
      },
      [](PPCContext* ctx) {
        auto result = ctx->v[3];
        // Lane 0: 1.0 + 1.0 = 2.0
        REQUIRE(result.u32[0] == 0x40000000);
        // Lane 1: src1 NaN propagated, quieted.
        REQUIRE((result.u32[1] & 0xFFC00000) == 0x7FC00000);
        REQUIRE((result.u32[1] & 0x003FFFFF) == 0x00001);
        // Lane 2: src2 NaN propagated, quieted.
        REQUIRE((result.u32[2] & 0xFFC00000) == 0x7FC00000);
        REQUIRE((result.u32[2] & 0x003FFFFF) == 0x00002);
        // Lane 3: generated NaN → PPC default.
        REQUIRE(result.u32[3] == 0xFFC00000);
      });
}
