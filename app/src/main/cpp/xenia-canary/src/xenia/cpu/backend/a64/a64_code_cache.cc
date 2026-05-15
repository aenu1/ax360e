/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/cpu/backend/a64/a64_code_cache.h"

#include "xenia/base/platform.h"
#if XE_PLATFORM_WIN32
#include "xenia/base/platform_win.h"
#endif
#if XE_PLATFORM_AX360E
#include "../aarch64_disasm.h"
#endif
namespace xe {
namespace cpu {
namespace backend {
namespace a64 {

bool A64CodeCache::Initialize() { return CodeCacheBase::Initialize(); }

void A64CodeCache::FillCode(void* write_address, size_t size) {
  // Fill with BRK #0 (0xD4200000), 4-byte aligned.
  constexpr uint32_t kBrk0 = 0xD4200000;
  auto* p = reinterpret_cast<uint32_t*>(write_address);
  auto* end =
      reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(write_address) + size);
  for (; p < end; ++p) {
    *p = kBrk0;
  }
}

void A64CodeCache::FlushCodeRange(void* address, size_t size) {
#if XE_PLATFORM_WIN32
  FlushInstructionCache(GetCurrentProcess(), address, size);
#else


#if XE_PLATFORM_AX360E
    //XELOGI("ASM:\n{}", aarch64_disasm(reinterpret_cast<uint64_t>(address),reinterpret_cast<uint32_t*>(address),size/4));
#endif
    __builtin___clear_cache(
      reinterpret_cast<char*>(address),
      reinterpret_cast<char*>(static_cast<uint8_t*>(address) + size));
#endif
}

}  // namespace a64
}  // namespace backend
}  // namespace cpu
}  // namespace xe
