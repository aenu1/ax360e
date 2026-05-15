/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <string>

#include "xenia/base/system.h"

namespace xe {
    void ShowSimpleMessageBox(SimpleMessageBoxType type, std::string_view message) {
    }

    void LaunchFileExplorer(const std::filesystem::path& path) {}

    void LaunchWebBrowser(const std::string_view url) {}

    bool SetProcessPriorityClass(const uint32_t priority_class) { return true; }

    bool IsUseNexusForGameBarEnabled() { return false; }
}