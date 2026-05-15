/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/cpu/testing/util.h"

#include <cstring>

using namespace xe;
using namespace xe::cpu;
using namespace xe::cpu::hir;
using namespace xe::cpu::testing;
using xe::cpu::ppc::PPCContext;

// =============================================================================
// Guest addresses are 32-bit. If a GPR holding a guest address has stale
// upper 32 bits, the backend must mask them before adding the host membase.
// Otherwise the final host pointer escapes the guest address space.
// =============================================================================

TEST_CASE("LOAD_I32_STALE_UPPER_BITS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    auto addr = LoadGPR(b, 4);
    StoreGPR(b, 3, b.ZeroExtend(b.Load(addr, INT32_TYPE), INT64_TYPE));
    b.Return();
  });
  test.Run(
      [&test](PPCContext* ctx) {
        uint32_t addr = test.memory->SystemHeapAlloc(4, 4);
        auto* host = test.memory->TranslateVirtual(addr);
        uint32_t sentinel = 0xCAFEBABE;
        std::memcpy(host, &sentinel, 4);
        // Set the GPR to the valid address with garbage upper bits.
        ctx->r[4] = 0xDEAD000000000000ULL | addr;
      },
      [&test](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xCAFEBABE);
        test.memory->SystemHeapFree(
            static_cast<uint32_t>(ctx->r[4] & 0xFFFFFFFF));
      });
}

TEST_CASE("STORE_I32_STALE_UPPER_BITS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    auto addr = LoadGPR(b, 4);
    auto val = b.Truncate(LoadGPR(b, 5), INT32_TYPE);
    b.Store(addr, val);
    b.Return();
  });
  test.Run(
      [&test](PPCContext* ctx) {
        uint32_t addr = test.memory->SystemHeapAlloc(4, 4);
        std::memset(test.memory->TranslateVirtual(addr), 0, 4);
        ctx->r[4] = 0xDEAD000000000000ULL | addr;
        ctx->r[5] = 0x12345678;
      },
      [&test](PPCContext* ctx) {
        uint32_t addr = static_cast<uint32_t>(ctx->r[4] & 0xFFFFFFFF);
        auto* host = test.memory->TranslateVirtual(addr);
        uint32_t result;
        std::memcpy(&result, host, 4);
        REQUIRE(result == 0x12345678);
        test.memory->SystemHeapFree(addr);
      });
}

// =============================================================================
// Guest address arithmetic must wrap at 32 bits. Test by computing
// (base + offset) in HIR where the 32-bit sum wraps, then loading/storing.
// =============================================================================

TEST_CASE("LOAD_I32_ADDRESS_WRAPS_AT_32_BITS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    // Compute guest address as (r4 + r5) truncated to 32 bits, then load.
    auto base = b.Truncate(LoadGPR(b, 4), INT32_TYPE);
    auto offset = b.Truncate(LoadGPR(b, 5), INT32_TYPE);
    auto addr = b.ZeroExtend(b.Add(base, offset), INT64_TYPE);
    StoreGPR(b, 3, b.ZeroExtend(b.Load(addr, INT32_TYPE), INT64_TYPE));
    b.Return();
  });
  test.Run(
      [&test](PPCContext* ctx) {
        uint32_t target_addr = test.memory->SystemHeapAlloc(4, 4);
        auto* host = test.memory->TranslateVirtual(target_addr);
        uint32_t sentinel = 0xDEADF00D;
        std::memcpy(host, &sentinel, 4);

        // base + offset overflows 32 bits and wraps to target_addr.
        ctx->r[4] = 0xFFFF0000u;
        ctx->r[5] = static_cast<uint64_t>(target_addr) + 0x10000u;
      },
      [&test](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xDEADF00D);
        uint32_t target_addr = static_cast<uint32_t>(
            0xFFFF0000u + static_cast<uint32_t>(ctx->r[5]));
        test.memory->SystemHeapFree(target_addr);
      });
}

TEST_CASE("STORE_I32_ADDRESS_WRAPS_AT_32_BITS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    auto base = b.Truncate(LoadGPR(b, 4), INT32_TYPE);
    auto offset = b.Truncate(LoadGPR(b, 5), INT32_TYPE);
    auto addr = b.ZeroExtend(b.Add(base, offset), INT64_TYPE);
    auto val = b.Truncate(LoadGPR(b, 6), INT32_TYPE);
    b.Store(addr, val);
    b.Return();
  });
  test.Run(
      [&test](PPCContext* ctx) {
        uint32_t target_addr = test.memory->SystemHeapAlloc(4, 4);
        std::memset(test.memory->TranslateVirtual(target_addr), 0, 4);

        ctx->r[4] = 0xFFFF0000u;
        ctx->r[5] = static_cast<uint64_t>(target_addr) + 0x10000u;
        ctx->r[6] = 0xBAADF00D;
      },
      [&test](PPCContext* ctx) {
        uint32_t target_addr = static_cast<uint32_t>(
            0xFFFF0000u + static_cast<uint32_t>(ctx->r[5]));
        auto* host = test.memory->TranslateVirtual(target_addr);
        uint32_t result;
        std::memcpy(&result, host, 4);
        REQUIRE(result == 0xBAADF00D);
        test.memory->SystemHeapFree(target_addr);
      });
}

// =============================================================================
// LOAD_OFFSET with constant offset and stale upper bits in base.
// =============================================================================

TEST_CASE("LOAD_OFFSET_I32_STALE_UPPER_BITS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    auto base = LoadGPR(b, 4);
    auto offset = b.LoadConstantInt64(4);
    StoreGPR(b, 3,
             b.ZeroExtend(b.LoadOffset(base, offset, INT32_TYPE), INT64_TYPE));
    b.Return();
  });
  test.Run(
      [&test](PPCContext* ctx) {
        uint32_t addr = test.memory->SystemHeapAlloc(8, 4);
        auto* host = test.memory->TranslateVirtual(addr + 4);
        uint32_t sentinel = 0x87654321;
        std::memcpy(host, &sentinel, 4);

        // Garbage upper bits in the base register.
        ctx->r[4] = 0xBEEF000000000000ULL | addr;
      },
      [&test](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x87654321);
        uint32_t addr = static_cast<uint32_t>(ctx->r[4] & 0xFFFFFFFF);
        test.memory->SystemHeapFree(addr);
      });
}

TEST_CASE("STORE_OFFSET_I32_STALE_UPPER_BITS", "[instr]") {
  TestFunction test([](HIRBuilder& b) {
    auto base = LoadGPR(b, 4);
    auto offset = b.LoadConstantInt64(4);
    auto val = b.Truncate(LoadGPR(b, 5), INT32_TYPE);
    b.StoreOffset(base, offset, val);
    b.Return();
  });
  test.Run(
      [&test](PPCContext* ctx) {
        uint32_t addr = test.memory->SystemHeapAlloc(8, 4);
        std::memset(test.memory->TranslateVirtual(addr), 0, 8);

        ctx->r[4] = 0xBEEF000000000000ULL | addr;
        ctx->r[5] = 0xFEEDFACE;
      },
      [&test](PPCContext* ctx) {
        uint32_t addr = static_cast<uint32_t>(ctx->r[4] & 0xFFFFFFFF);
        auto* host = test.memory->TranslateVirtual(addr + 4);
        uint32_t result;
        std::memcpy(&result, host, 4);
        REQUIRE(result == 0xFEEDFACE);
        test.memory->SystemHeapFree(addr);
      });
}
