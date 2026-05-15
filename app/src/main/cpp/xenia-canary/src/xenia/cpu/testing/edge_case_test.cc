/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

// Tests for ARM64-sensitive edge cases: sign-extension, byte-swap, extract
// with high-bit values, rounding modes, and NaN handling.

#include "xenia/cpu/testing/util.h"

#include <cmath>
#include <cstring>

using namespace xe;
using namespace xe::cpu;
using namespace xe::cpu::hir;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

// ============================================================================
// BYTE_SWAP scalar — I16, I32, I64
// ============================================================================
TEST_CASE("BYTE_SWAP_I16", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.ByteSwap(b.Truncate(LoadGPR(b, 4), INT16_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0x1234; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint16_t>(ctx->r[3]) == 0x3412);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0x0100; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint16_t>(ctx->r[3]) == 0x0001);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0x80FF; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint16_t>(ctx->r[3]) == 0xFF80);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0xFFFF; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint16_t>(ctx->r[3]) == 0xFFFF);
           });
}

TEST_CASE("BYTE_SWAP_I32", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.ByteSwap(b.Truncate(LoadGPR(b, 4), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0x01020304; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x04030201);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0x80000000; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x00000080);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0xFF000000; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x000000FF);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0xDEADBEEF; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xEFBEADDE);
           });
  // Verify upper 32 bits are zero.
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0xFFFFFFFF; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xFFFFFFFF);
             REQUIRE((ctx->r[3] >> 32) == 0);
           });
}

TEST_CASE("BYTE_SWAP_I64", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3, b.ByteSwap(LoadGPR(b, 4)));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x0102030405060708ULL; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x0807060504030201ULL); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x8000000000000000ULL; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x0000000000000080ULL); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0xFF00000000000000ULL; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x00000000000000FFULL); });
}

// ============================================================================
// EXTRACT with high-bit-set values
// Tests UMOV vs SMOV — must zero-extend, not sign-extend.
// ============================================================================
TEST_CASE("EXTRACT_INT8_HIGHBIT", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.Extract(LoadVR(b, 4),
                               b.Truncate(LoadGPR(b, 4), INT8_TYPE), INT8_TYPE),
                     INT64_TYPE));
    b.Return();
  });
  // Extract 0xFF — must be 0xFF (255), NOT 0xFFFFFFFFFFFFFFFF (-1).
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;  // index 0
        ctx->v[4] =
            vec128b(0xFF, 0x80, 0x7F, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      },
      [](PPCContext* ctx) {
        REQUIRE(ctx->r[3] == 0xFF);  // NOT sign-extended
      });
  // Extract 0x80
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->v[4] =
            vec128b(0xFF, 0x80, 0x7F, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      },
      [](PPCContext* ctx) {
        REQUIRE(ctx->r[3] == 0x80);  // NOT sign-extended
      });
}

TEST_CASE("EXTRACT_INT16_HIGHBIT", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(
                 b.Extract(LoadVR(b, 4), b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                           INT16_TYPE),
                 INT64_TYPE));
    b.Return();
  });
  // Extract 0x8000 — must NOT sign-extend to 0xFFFFFFFFFFFF8000.
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->v[4] = vec128s(0x8000, 0xFFFF, 0x7FFF, 0x0001, 0, 0, 0, 0);
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x8000); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->v[4] = vec128s(0x8000, 0xFFFF, 0x7FFF, 0x0001, 0, 0, 0, 0);
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0xFFFF); });
}

TEST_CASE("EXTRACT_INT32_HIGHBIT", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(
                 b.Extract(LoadVR(b, 4), b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                           INT32_TYPE),
                 INT64_TYPE));
    b.Return();
  });
  // Extract 0x80000000 — must NOT sign-extend to 0xFFFFFFFF80000000.
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->v[4] = vec128i(0x80000000, 0xFFFFFFFF, 0x7FFFFFFF, 0x00000001);
      },
      [](PPCContext* ctx) {
        REQUIRE(ctx->r[3] == 0x80000000ULL);  // upper bits must be 0
      });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->v[4] = vec128i(0x80000000, 0xFFFFFFFF, 0x7FFFFFFF, 0x00000001);
      },
      [](PPCContext* ctx) {
        REQUIRE(ctx->r[3] == 0xFFFFFFFFULL);  // upper bits must be 0
      });
}

// ============================================================================
// Memory: store then load byte-swap round-trip for I32
// Store/Load use guest addresses (membase-relative).
// ============================================================================
TEST_CASE("STORE_LOAD_BYTESWAP_I32", "[memory]") {
  TestFunction test([](HIRBuilder& b) {
    // Store r[5] (truncated to I32) at guest address r[4] with byte-swap.
    auto addr = LoadGPR(b, 4);
    auto val = b.Truncate(LoadGPR(b, 5), INT32_TYPE);
    b.Store(addr, val, LOAD_STORE_BYTE_SWAP);
    // Load it back with byte-swap — should get original value.
    auto loaded = b.Load(addr, INT32_TYPE, LOAD_STORE_BYTE_SWAP);
    StoreGPR(b, 3, b.ZeroExtend(loaded, INT64_TYPE));
    b.Return();
  });

  uint32_t guest_addr = test.memory->SystemHeapAlloc(4);
  REQUIRE(guest_addr != 0);

  test.Run(
      [&](PPCContext* ctx) {
        ctx->r[4] = guest_addr;
        ctx->r[5] = 0xDEADBEEF;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xDEADBEEF);
      });
  test.Run(
      [&](PPCContext* ctx) {
        ctx->r[4] = guest_addr;
        ctx->r[5] = 0x80000001;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x80000001);
      });

  test.memory->SystemHeapFree(guest_addr);
}

// ============================================================================
// SET_ROUNDING_MODE — mode 3 (toward -infinity / floor)
// ============================================================================
TEST_CASE("SET_ROUNDING_MODE_3", "[backend]") {
  TestFunction test([](HIRBuilder& b) {
    auto a = b.Convert(LoadFPR(b, 4), FLOAT32_TYPE);
    auto c = b.Convert(LoadFPR(b, 5), FLOAT32_TYPE);
    auto sum = b.Add(a, c);
    StoreFPR(b, 3, b.Convert(sum, FLOAT64_TYPE));
    b.Return();
  });

  // Mode 3 = toward -infinity (floor).
  // 1.0 + epsilon: floor rounds down to 1.0.
  test.Run(
      [&test](PPCContext* ctx) {
        ctx->f[4] = 1.0;
        ctx->f[5] = std::ldexp(1.0, -24);
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 3);
      },
      [&test](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        REQUIRE(result == 1.0f);
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 0);
      });

  // -1.0 + -epsilon: floor rounds toward more negative.
  test.Run(
      [&test](PPCContext* ctx) {
        ctx->f[4] = -1.0;
        ctx->f[5] = -std::ldexp(1.0, -24);
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 3);
      },
      [&test](PPCContext* ctx) {
        auto result = static_cast<float>(ctx->f[3]);
        float expected = std::nextafterf(-1.0f, -2.0f);
        REQUIRE(result == expected);
        test.processors[0]->backend()->SetGuestRoundingMode(ctx, 0);
      });
}

// ============================================================================
// Truncate with garbage in upper register bits
// Ensures I8/I16 Truncate properly masks on ARM64 where WRegs are 32-bit.
// ============================================================================
TEST_CASE("ADD_I8_UPPER_BITS_GARBAGE", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Add(b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // Upper bytes are garbage — Truncate must isolate low byte.
  // 0x80 + 0x01 = 0x81 (only low bytes matter)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xDEADBA80;  // low byte = 0x80
        ctx->r[5] = 0xCAFE0001;  // low byte = 0x01
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0x81);
      });
  // 0xFF + 0x01 = 0x00 (overflow wraps at 8 bits)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x000100FF;
        ctx->r[5] = 0x00010001;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0x00);
      });
}

TEST_CASE("ADD_I16_UPPER_BITS_GARBAGE", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Add(b.Truncate(LoadGPR(b, 4), INT16_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT16_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // 0x8000 + 0x0001 = 0x8001
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xDEAD8000;
        ctx->r[5] = 0xCAFE0001;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint16_t>(ctx->r[3]) == 0x8001);
      });
}

// ============================================================================
// SHA I8 additional edge cases
// ============================================================================
TEST_CASE("SHA_I8_EDGE", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Sha(b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // 0x80 >> 7 = 0xFF (MSB=1, arithmetic shift fills with 1s)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80;
        ctx->r[5] = 7;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0xFF);
      });
  // 0x01 >> 1 = 0x00 (positive, shift right)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x01;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0x00);
      });
  // 0xC0 >> 1 = 0xE0 (negative, sign bit fills)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xC0;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0xE0);
      });
}

// ============================================================================
// SHR I8 distinguishing from SHA (logical vs arithmetic)
// ============================================================================
TEST_CASE("SHR_I8_VS_SHA", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Shr(b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // 0x80 >> 7 = 0x01 (logical: zero fills from left)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80;
        ctx->r[5] = 7;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0x01);
      });
  // 0x80 >> 1 = 0x40 (logical)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0x40);
      });
  // 0xFF >> 1 = 0x7F (logical, not 0xFF which SHA would give)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFF;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0x7F);
      });
}
