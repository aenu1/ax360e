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

// Helper: build a compare test function for a given type width and compare op.
// Loads r[4] and r[5] as the given integer type, compares, stores result
// (0 or 1) in r[3].

// ============================================================================
// COMPARE_EQ
// ============================================================================
TEST_CASE("COMPARE_EQ_I8", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareEQ(b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                                      b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFF;
        ctx->r[5] = 0xFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
}

TEST_CASE("COMPARE_EQ_I16", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareEQ(b.Truncate(LoadGPR(b, 4), INT16_TYPE),
                                      b.Truncate(LoadGPR(b, 5), INT16_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x8000;
        ctx->r[5] = 0x8000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

TEST_CASE("COMPARE_EQ_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareEQ(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                      b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0xFFFFFFFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 2;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

TEST_CASE("COMPARE_EQ_I64", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.CompareEQ(LoadGPR(b, 4), LoadGPR(b, 5)), INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFFFFFFFFFFULL;
        ctx->r[5] = 0xFFFFFFFFFFFFFFFFULL;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // Same low 32 bits, different upper — must NOT be equal.
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x0000000080000000ULL;
        ctx->r[5] = 0xFFFFFFFF80000000ULL;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_NE
// ============================================================================
TEST_CASE("COMPARE_NE_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareNE(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                      b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 2;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_SLT (signed less than)
// ============================================================================
TEST_CASE("COMPARE_SLT_I8", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareSLT(b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // -1 < 0 (signed)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0 < -1 (signed) = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0xFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  // Equal = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 5;
        ctx->r[5] = 5;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

TEST_CASE("COMPARE_SLT_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareSLT(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // -1 < 0
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0 < -1 = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0xFFFFFFFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  // INT_MIN < 0
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0 < INT_MAX
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0x7FFFFFFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // INT_MAX < INT_MIN = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x7FFFFFFF;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  // Equal = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 42;
        ctx->r[5] = 42;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

TEST_CASE("COMPARE_SLT_I64", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.CompareSLT(LoadGPR(b, 4), LoadGPR(b, 5)), INT64_TYPE));
    b.Return();
  });
  // -1 < 0
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFFFFFFFFFFULL;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // Sign-extended 32-bit: 0xFFFFFFFF80000000 as signed i64 is negative.
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF80000000ULL;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0x0000000080000000 as signed i64 is positive (2^31).
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x0000000080000000ULL;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_SLE (signed less or equal)
// ============================================================================
TEST_CASE("COMPARE_SLE_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareSLE(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_SGT (signed greater than)
// ============================================================================
TEST_CASE("COMPARE_SGT_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareSGT(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0xFFFFFFFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_SGE (signed greater or equal)
// ============================================================================
TEST_CASE("COMPARE_SGE_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareSGE(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0xFFFFFFFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
}

// ============================================================================
// COMPARE_ULT (unsigned less than)
// ============================================================================
TEST_CASE("COMPARE_ULT_I8", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareULT(b.Truncate(LoadGPR(b, 4), INT8_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // 0 < 0xFF (unsigned: 0 < 255)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0xFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0xFF < 0 (unsigned: 255 < 0 = false)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

TEST_CASE("COMPARE_ULT_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareULT(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // 0 < 0x80000000 (unsigned: 0 < 2^31)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0x80000000 < 0 (unsigned: 2^31 < 0 = false)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  // 0xFFFFFFFF < 1 (unsigned: MAX < 1 = false)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  // 1 < 0xFFFFFFFF (unsigned)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 0xFFFFFFFF;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // Equal = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

TEST_CASE("COMPARE_ULT_I64", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.CompareULT(LoadGPR(b, 4), LoadGPR(b, 5)), INT64_TYPE));
    b.Return();
  });
  // 0 < 1
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0x0000000080000000 < 0xFFFFFFFF80000000 (unsigned: yes)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x0000000080000000ULL;
        ctx->r[5] = 0xFFFFFFFF80000000ULL;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0xFFFFFFFF80000000 < 0x0000000080000000 (unsigned: no)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF80000000ULL;
        ctx->r[5] = 0x0000000080000000ULL;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_ULE (unsigned less or equal)
// ============================================================================
TEST_CASE("COMPARE_ULE_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareULE(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_UGT (unsigned greater than)
// ============================================================================
TEST_CASE("COMPARE_UGT_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareUGT(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  // 0x80000000 > 0 (unsigned: 2^31 > 0 = true)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // 0 > 0x80000000 (unsigned: false)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0x80000000;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
  // 0xFFFFFFFF > 0 (unsigned: MAX > 0 = true)
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  // Equal = false
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 5;
        ctx->r[5] = 5;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// COMPARE_UGE (unsigned greater or equal)
// ============================================================================
TEST_CASE("COMPARE_UGE_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.CompareUGE(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0); });
}

// ============================================================================
// SELECT — conditional select based on I8 condition
// ============================================================================
TEST_CASE("SELECT_I32", "[compare]") {
  TestFunction test([](HIRBuilder& b) {
    auto cond = b.Truncate(LoadGPR(b, 4), INT8_TYPE);
    auto val_true = b.Truncate(LoadGPR(b, 5), INT32_TYPE);
    auto val_false = b.Truncate(LoadGPR(b, 6), INT32_TYPE);
    StoreGPR(b, 3,
             b.ZeroExtend(b.Select(cond, val_true, val_false), INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 100;
        ctx->r[6] = 200;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 100);
      });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 100;
        ctx->r[6] = 200;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 200);
      });
}

// ============================================================================
// BRANCH_TRUE / BRANCH_FALSE — test that comparisons feed branches correctly
// ============================================================================
// Branch tests use TestModule with skip_cf_simplification=true because
// manually-constructed HIR branches don't set up block structure the same
// way the PPC frontend does, and CF simplification mishandles them.
TEST_CASE("BRANCH_ON_COMPARE_SLT_I32", "[compare]") {
  auto memory = std::make_unique<Memory>();
  memory->Initialize();

  std::unique_ptr<xe::cpu::backend::Backend> backend;
#if XE_ARCH_AMD64
  backend.reset(new xe::cpu::backend::x64::X64Backend());
#elif XE_ARCH_ARM64
  backend.reset(new xe::cpu::backend::a64::A64Backend());
#endif
  REQUIRE(backend);

  auto processor = std::make_unique<Processor>(memory.get(), nullptr);
  processor->Setup(std::move(backend));

  auto module = std::make_unique<TestModule>(
      processor.get(), "Test",
      [](uint32_t address) { return address == 0x80000000; },
      [](HIRBuilder& b) {
        auto cmp = b.CompareSLT(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT32_TYPE));
        auto* if_true = b.NewLabel();
        auto* if_false = b.NewLabel();
        auto* end = b.NewLabel();
        b.BranchTrue(cmp, if_true);
        b.MarkLabel(if_false);
        StoreGPR(b, 3, b.LoadConstantUint64(0));
        b.Branch(end);
        b.MarkLabel(if_true);
        StoreGPR(b, 3, b.LoadConstantUint64(1));
        b.Branch(end);
        b.MarkLabel(end);
        b.Return();
        return true;
      },
      /*skip_cf_simplification=*/true);
  processor->AddModule(std::move(module));
  processor->backend()->CommitExecutableRange(0x80000000, 0x80010000);

  auto fn = processor->ResolveFunction(0x80000000);
  REQUIRE(fn != nullptr);

  uint32_t stack_size = 64 * 1024;
  uint32_t stack_address = memory->SystemHeapAlloc(stack_size);

  auto run = [&](uint64_t r4, uint64_t r5, uint64_t expected_r3) {
    auto thread_state = std::make_unique<ThreadState>(
        processor.get(), 0x100, stack_address + stack_size);
    auto ctx = thread_state->context();
    ctx->lr = 0xBCBCBCBC;
    ctx->r[3] = 99;
    ctx->r[4] = r4;
    ctx->r[5] = r5;
    fn->Call(thread_state.get(), uint32_t(ctx->lr));
    REQUIRE(ctx->r[3] == expected_r3);
  };

  run(0xFFFFFFFF, 0, 1);  // -1 < 0 = true
  run(0, 0xFFFFFFFF, 0);  // 0 < -1 = false
  run(0x80000000, 0, 1);  // INT_MIN < 0 = true
  run(0, 0x80000000, 0);  // 0 < INT_MIN = false
  run(42, 42, 0);         // equal = false

  memory->SystemHeapFree(stack_address);
}

TEST_CASE("BRANCH_ON_COMPARE_UGT_I32", "[compare]") {
  auto memory = std::make_unique<Memory>();
  memory->Initialize();

  std::unique_ptr<xe::cpu::backend::Backend> backend;
#if XE_ARCH_AMD64
  backend.reset(new xe::cpu::backend::x64::X64Backend());
#elif XE_ARCH_ARM64
  backend.reset(new xe::cpu::backend::a64::A64Backend());
#endif
  REQUIRE(backend);

  auto processor = std::make_unique<Processor>(memory.get(), nullptr);
  processor->Setup(std::move(backend));

  auto module = std::make_unique<TestModule>(
      processor.get(), "Test",
      [](uint32_t address) { return address == 0x80000000; },
      [](HIRBuilder& b) {
        auto cmp = b.CompareUGT(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT32_TYPE));
        auto* if_true = b.NewLabel();
        auto* if_false = b.NewLabel();
        auto* end = b.NewLabel();
        b.BranchTrue(cmp, if_true);
        b.MarkLabel(if_false);
        StoreGPR(b, 3, b.LoadConstantUint64(0));
        b.Branch(end);
        b.MarkLabel(if_true);
        StoreGPR(b, 3, b.LoadConstantUint64(1));
        b.Branch(end);
        b.MarkLabel(end);
        b.Return();
        return true;
      },
      /*skip_cf_simplification=*/true);
  processor->AddModule(std::move(module));
  processor->backend()->CommitExecutableRange(0x80000000, 0x80010000);

  auto fn = processor->ResolveFunction(0x80000000);
  REQUIRE(fn != nullptr);

  uint32_t stack_size = 64 * 1024;
  uint32_t stack_address = memory->SystemHeapAlloc(stack_size);

  auto run = [&](uint64_t r4, uint64_t r5, uint64_t expected_r3) {
    auto thread_state = std::make_unique<ThreadState>(
        processor.get(), 0x100, stack_address + stack_size);
    auto ctx = thread_state->context();
    ctx->lr = 0xBCBCBCBC;
    ctx->r[3] = 99;
    ctx->r[4] = r4;
    ctx->r[5] = r5;
    fn->Call(thread_state.get(), uint32_t(ctx->lr));
    REQUIRE(ctx->r[3] == expected_r3);
  };

  run(0x80000000, 0, 1);           // 2^31 > 0 = true
  run(0, 0x80000000, 0);           // 0 > 2^31 = false
  run(0xFFFFFFFF, 0xFFFFFFFE, 1);  // MAX > MAX-1 = true
  run(5, 5, 0);                    // equal = false

  memory->SystemHeapFree(stack_address);
}

// ============================================================================
// BRANCH_TRUE F32 — float-conditioned branch (nonzero bits = true)
// ============================================================================
TEST_CASE("BRANCH_ON_F32_TRUE", "[compare]") {
  auto memory = std::make_unique<Memory>();
  memory->Initialize();

  std::unique_ptr<xe::cpu::backend::Backend> backend;
#if XE_ARCH_AMD64
  backend.reset(new xe::cpu::backend::x64::X64Backend());
#elif XE_ARCH_ARM64
  backend.reset(new xe::cpu::backend::a64::A64Backend());
#endif
  REQUIRE(backend);

  auto processor = std::make_unique<Processor>(memory.get(), nullptr);
  processor->Setup(std::move(backend));

  auto module = std::make_unique<TestModule>(
      processor.get(), "Test",
      [](uint32_t address) { return address == 0x80000000; },
      [](HIRBuilder& b) {
        // Branch if f[4] (as F32) is nonzero.
        auto cond = b.Convert(LoadFPR(b, 4), FLOAT32_TYPE);
        auto* if_true = b.NewLabel();
        auto* if_false = b.NewLabel();
        auto* end = b.NewLabel();
        b.BranchTrue(cond, if_true);
        b.MarkLabel(if_false);
        StoreGPR(b, 3, b.LoadConstantUint64(0));
        b.Branch(end);
        b.MarkLabel(if_true);
        StoreGPR(b, 3, b.LoadConstantUint64(1));
        b.Branch(end);
        b.MarkLabel(end);
        b.Return();
        return true;
      },
      /*skip_cf_simplification=*/true);
  processor->AddModule(std::move(module));
  processor->backend()->CommitExecutableRange(0x80000000, 0x80010000);

  auto fn = processor->ResolveFunction(0x80000000);
  REQUIRE(fn != nullptr);

  uint32_t stack_size = 64 * 1024;
  uint32_t stack_address = memory->SystemHeapAlloc(stack_size);

  auto run = [&](double f4, uint64_t expected_r3) {
    auto thread_state = std::make_unique<ThreadState>(
        processor.get(), 0x100, stack_address + stack_size);
    auto ctx = thread_state->context();
    ctx->lr = 0xBCBCBCBC;
    ctx->r[3] = 99;
    ctx->f[4] = f4;
    fn->Call(thread_state.get(), uint32_t(ctx->lr));
    REQUIRE(ctx->r[3] == expected_r3);
  };

  run(1.0, 1);   // nonzero → true
  run(0.0, 0);   // zero → false
  run(-1.0, 1);  // nonzero → true
  run(-0.0, 1);  // -0.0f = 0x80000000 (sign bit set), nonzero bits → taken

  memory->SystemHeapFree(stack_address);
}

// ============================================================================
// BRANCH_FALSE F32 — float-conditioned branch (zero bits = true)
// ============================================================================
TEST_CASE("BRANCH_ON_F32_FALSE", "[compare]") {
  auto memory = std::make_unique<Memory>();
  memory->Initialize();

  std::unique_ptr<xe::cpu::backend::Backend> backend;
#if XE_ARCH_AMD64
  backend.reset(new xe::cpu::backend::x64::X64Backend());
#elif XE_ARCH_ARM64
  backend.reset(new xe::cpu::backend::a64::A64Backend());
#endif
  REQUIRE(backend);

  auto processor = std::make_unique<Processor>(memory.get(), nullptr);
  processor->Setup(std::move(backend));

  auto module = std::make_unique<TestModule>(
      processor.get(), "Test",
      [](uint32_t address) { return address == 0x80000000; },
      [](HIRBuilder& b) {
        auto cond = b.Convert(LoadFPR(b, 4), FLOAT32_TYPE);
        auto* if_false = b.NewLabel();
        auto* if_not = b.NewLabel();
        auto* end = b.NewLabel();
        b.BranchFalse(cond, if_false);
        b.MarkLabel(if_not);
        StoreGPR(b, 3, b.LoadConstantUint64(0));
        b.Branch(end);
        b.MarkLabel(if_false);
        StoreGPR(b, 3, b.LoadConstantUint64(1));
        b.Branch(end);
        b.MarkLabel(end);
        b.Return();
        return true;
      },
      /*skip_cf_simplification=*/true);
  processor->AddModule(std::move(module));
  processor->backend()->CommitExecutableRange(0x80000000, 0x80010000);

  auto fn = processor->ResolveFunction(0x80000000);
  REQUIRE(fn != nullptr);

  uint32_t stack_size = 64 * 1024;
  uint32_t stack_address = memory->SystemHeapAlloc(stack_size);

  auto run = [&](double f4, uint64_t expected_r3) {
    auto thread_state = std::make_unique<ThreadState>(
        processor.get(), 0x100, stack_address + stack_size);
    auto ctx = thread_state->context();
    ctx->lr = 0xBCBCBCBC;
    ctx->r[3] = 99;
    ctx->f[4] = f4;
    fn->Call(thread_state.get(), uint32_t(ctx->lr));
    REQUIRE(ctx->r[3] == expected_r3);
  };

  run(0.0, 1);  // zero → branch taken
  run(1.0, 0);  // nonzero → not taken

  memory->SystemHeapFree(stack_address);
}

// ============================================================================
// AND, OR, XOR, NOT — basic bitwise ops
// ============================================================================
TEST_CASE("AND_I32", "[bitwise]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.And(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFF00FF00;
        ctx->r[5] = 0x0F0F0F0F;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x0F000F00);
      });
}

TEST_CASE("OR_I32", "[bitwise]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Or(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                               b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFF00FF00;
        ctx->r[5] = 0x00FF00FF;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xFFFFFFFF);
      });
}

TEST_CASE("XOR_I32", "[bitwise]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Xor(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 0xFF00FF00;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x00FF00FF);
      });
}

TEST_CASE("NOT_I32", "[bitwise]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.Not(b.Truncate(LoadGPR(b, 4), INT32_TYPE)), INT64_TYPE));
    b.Return();
  });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xFFFFFFFF);
           });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 0xFF00FF00; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x00FF00FF);
           });
}

// ============================================================================
// SUB
// ============================================================================
TEST_CASE("SUB_I32", "[arithmetic]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Sub(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 10;
        ctx->r[5] = 3;
      },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 7); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xFFFFFFFF);
      });
}

// ============================================================================
// MUL
// ============================================================================
TEST_CASE("MUL_I32", "[arithmetic]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Mul(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                b.Truncate(LoadGPR(b, 5), INT32_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 7;
        ctx->r[5] = 6;
      },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 42); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0xFFFFFFFF;
        ctx->r[5] = 2;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xFFFFFFFE);
      });
}

// ============================================================================
// NEG
// ============================================================================
TEST_CASE("NEG_I32", "[arithmetic]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.Neg(b.Truncate(LoadGPR(b, 4), INT32_TYPE)), INT64_TYPE));
    b.Return();
  });
  test.Run([](PPCContext* ctx) { ctx->r[4] = 1; },
           [](PPCContext* ctx) {
             REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0xFFFFFFFF);
           });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0xFFFFFFFF; },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 1); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0; },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0); });
}

// ============================================================================
// SIGN_EXTEND / ZERO_EXTEND
// ============================================================================
TEST_CASE("SIGN_EXTEND_I32_TO_I64", "[convert]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.SignExtend(b.Truncate(LoadGPR(b, 4), INT32_TYPE), INT64_TYPE));
    b.Return();
  });
  // Positive value — upper bits should be 0.
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x7FFFFFFF; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x000000007FFFFFFFULL); });
  // Negative value — upper bits should be 1.
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x80000000; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0xFFFFFFFF80000000ULL); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0xFFFFFFFF; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0xFFFFFFFFFFFFFFFFULL); });
}

TEST_CASE("ZERO_EXTEND_I32_TO_I64", "[convert]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.Truncate(LoadGPR(b, 4), INT32_TYPE), INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x80000000; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x0000000080000000ULL); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0xFFFFFFFF; },
      [](PPCContext* ctx) { REQUIRE(ctx->r[3] == 0x00000000FFFFFFFFULL); });
}

// ============================================================================
// CNTLZ
// ============================================================================
TEST_CASE("CNTLZ_I32", "[bitwise]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(
        b, 3,
        b.ZeroExtend(b.CountLeadingZeros(b.Truncate(LoadGPR(b, 4), INT32_TYPE)),
                     INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0; },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 32); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 1; },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 31); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x80000000; },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 0); });
  test.Run(
      [](PPCContext* ctx) { ctx->r[4] = 0x00010000; },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint8_t>(ctx->r[3]) == 15); });
}

// ============================================================================
// ROTATE_LEFT
// ============================================================================
TEST_CASE("ROTATE_LEFT_I32", "[bitwise]") {
  TestFunction test([](HIRBuilder& b) {
    StoreGPR(b, 3,
             b.ZeroExtend(b.RotateLeft(b.Truncate(LoadGPR(b, 4), INT32_TYPE),
                                       b.Truncate(LoadGPR(b, 5), INT8_TYPE)),
                          INT64_TYPE));
    b.Return();
  });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 1;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 2); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x80000000;
        ctx->r[5] = 1;
      },
      [](PPCContext* ctx) { REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 1); });
  test.Run(
      [](PPCContext* ctx) {
        ctx->r[4] = 0x12345678;
        ctx->r[5] = 0;
      },
      [](PPCContext* ctx) {
        REQUIRE(static_cast<uint32_t>(ctx->r[3]) == 0x12345678);
      });
}
