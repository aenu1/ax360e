/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/cpu/backend/x64/x64_code_cache.h"

#include <cstring>

#if ENABLE_VTUNE
#include "third_party/vtune/include/jitprofiling.h"
#pragma comment(lib, "../third_party/vtune/lib64/jitprofiling.lib")
#endif

#if ENABLE_VTUNE
#include "third_party/fmt/include/fmt/format.h"
#include "xenia/cpu/module.h"
#endif

namespace xe {
namespace cpu {
namespace backend {
namespace x64 {

bool X64CodeCache::Initialize() { return CodeCacheBase::Initialize(); }

void X64CodeCache::FillCode(void* write_address, size_t size) {
  std::memset(write_address, 0xCC, size);
}

void X64CodeCache::FlushCodeRange(void* address, size_t size) {
  // x86-64 has coherent I/D caches; no flush needed.
}

void X64CodeCache::OnCodePlaced(uint32_t guest_address,
                                GuestFunction* function_info,
                                void* code_execute_address, size_t code_size) {
#if ENABLE_VTUNE
  if (iJIT_IsProfilingActive() == iJIT_SAMPLING_ON) {
    std::string method_name;
    if (function_info && function_info->name().size() != 0) {
      method_name = function_info->name();
    } else {
      method_name = fmt::format("sub_{:08X}", guest_address);
    }

    iJIT_Method_Load_V2 method = {0};
    method.method_id = iJIT_GetNewMethodID();
    method.method_load_address = code_execute_address;
    method.method_size = uint32_t(code_size);
    method.method_name = const_cast<char*>(method_name.data());
    method.module_name = function_info
                             ? (char*)function_info->module()->name().c_str()
                             : nullptr;
    iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED_V2, (void*)&method);
  }
#endif
}

}  // namespace x64
}  // namespace backend
}  // namespace cpu
}  // namespace xe
