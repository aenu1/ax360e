#!/usr/bin/env python3

# Copyright 2025 Ben Vanik. All Rights Reserved.

"""Android test runner for ax360e.

Mirrors `python xenia-build.py test`, but instead of running the Catch2 test
executables on the host, it cross-compiles them for Android (arm64), pushes
them to a connected device via `adb`, and runs them there.

All generated artifacts (the CMake/Ninja build tree, per-test logs and an
overall summary) are written under `t/run_test/` at the project root.

Examples:
  # Build the default test suites, push and run them on the attached device:
  python run_test.py

  # Reuse an existing build, only push + run, keep going after a failure:
  python run_test.py --no_build --continue

  # Run only the base/kernel suites in Release, forwarding a Catch2 filter:
  python run_test.py --target=xenia-base-tests --target=xenia-kernel-tests \\
      --config=release -- --test-case=Foo*

Run with -h/--help for the full option list.
"""

import glob
import json
import os
import re
import shutil
import subprocess
import sys
import time
from argparse import ArgumentParser

__author__ = "ax360e"

self_path = os.path.dirname(os.path.abspath(__file__))

# xenia-canary sources live inside the Android app's native tree.
cpp_dir = os.path.join(self_path, "app", "src", "main", "cpp")
xenia_canary_dir = os.path.join(cpp_dir, "xenia-canary")

# Everything this script produces (build tree, logs, shim, summary) goes here.
output_root = os.path.join(self_path, "t", "run_test")
build_dir = os.path.join(output_root, "build")
logs_dir = os.path.join(output_root, "logs")
shim_dir = os.path.join(output_root, "shim")

# Where test binaries + shared deps are staged on the device.
device_dir = "/data/local/tmp/ax360e_run_test"

# The Catch2 test executables that `xenia-build.py test` runs on the host.
DEFAULT_TEST_TARGETS = [
    "xenia-base-tests",
    "xenia-cpu-tests",
    "xenia-kernel-tests",
]

# Maps an Android ABI to the LLVM target arch used to build the link shim.
ABI_TO_ARCH = {
    "arm64-v8a": "aarch64",
    "armeabi-v7a": "arm",
    "x86_64": "x86_64",
    "x86": "i686",
}

# Source for the Android-only link shim (see build_link_shim).
#
# xenia's shared sources reference a handful of xe:: system entry points but
# only implement them in the *_android.cc / *_win.cc translation units. On
# Android those platform units are NOT part of the xenia-base static library
# (xe_platform_sources only re-adds the *_posix.* files), so the Catch2 test
# executables link with these no-op stubs instead. This lives entirely under
# t/run_test and never touches the shared xenia-canary sources, keeping the
# Windows build and the app's libe.so target unaffected.
SHIM_SOURCE = r"""
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace xe {

enum class SimpleMessageBoxType {
  Help,
  Warning,
  Error,
};

bool InitializeAndroidSystemForApplicationContext() { return true; }
void ShutdownAndroidSystem() {}

void LaunchWebBrowser(const std::string_view) {}
void LaunchFileExplorer(const std::filesystem::path&) {}

bool SetProcessPriorityClass(const uint32_t) { return true; }

bool IsUseNexusForGameBarEnabled() { return false; }

void ShowSimpleMessageBox(SimpleMessageBoxType, std::string_view) {}

}  // namespace xe
"""

# Cross-translation-unit globals the app defines in the heavy JNI bootstrap
# (emulator_ax360e.cpp). The CPU-test link pulls in the app's SAF/DocumentFile
# sources below, which reference this global; the tests never populate it, so a
# null definition here resolves the symbol without dragging in the Android
# application startup. Lives entirely under t/run_test.
APP_BRIDGE_SOURCE = r"""
#include <jni.h>

// Defined in emulator_ax360e.cpp for the real app; referenced by
// document_file.cpp. The test binary never runs the JNI tree.
jobject g_doocument_file_tree = nullptr;
jclass g_class_Emulator = nullptr;
jobject g_context = nullptr;
jmethodID mid_open_uri_fd = nullptr;
"""

# Real app translation units cross-compiled into the test link so the
# Android-only symbols emulator.cc references under XE_PLATFORM_AX360E (the SAF
# virtual file-system devices, DocumentFile and FilePicker) resolve with their
# actual implementations. These sources normally only enter the libe.so 'emu'
# target; a standalone Catch2 test binary does not link them.
APP_BRIDGE_SOURCES = [
    "document_file.cpp",
    "xe_saf_xex_device.cpp",
    "xe_saf_xex_entry.cpp",
    "xe_saf_xex_file.cpp",
    "xe_saf_stfs_device.cpp",
    "xe_saf_stfs_entry.cpp",
    "xe_saf_stfs_file.cpp",
    "xe_saf_disc_image_device.cpp",
    "xe_saf_disc_image_entry.cpp",
    "xe_saf_disc_image_file.cpp",
    "xe_saf_disc_archive_device.cpp",
    "xe_saf_disc_archive_entry.cpp",
    "xe_saf_disc_archive_file.cpp",
    "xenia-canary/src/xenia/ui/file_picker_android.cc",
]


class bcolors:
    OKCYAN = "\033[96m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"


def print_error(text):
    print(f"{bcolors.FAIL}ERROR: {text}{bcolors.ENDC}")


def print_warning(text):
    print(f"{bcolors.WARNING}WARNING: {text}{bcolors.ENDC}")


def print_status(text):
    print(f"{bcolors.OKCYAN}{text}{bcolors.ENDC}")


def host_tag():
    """Returns the NDK prebuilt host tag for the current OS."""
    if os.name == "nt":
        return "windows-x86_64"
    if sys.platform == "darwin":
        return "darwin-x86_64"
    return "linux-x86_64"


# ---------------------------------------------------------------------------
# Toolchain / SDK discovery
# ---------------------------------------------------------------------------

def read_local_properties():
    """Parses local.properties at the project root into a dict."""
    props = {}
    path = os.path.join(self_path, "local.properties")
    if os.path.isfile(path):
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or "=" not in line:
                    continue
                key, value = line.split("=", 1)
                # Unescape Java properties sequences (e.g. C\:\\ -> C:\).
                value = value.replace("\\:", ":").replace("\\\\", "\\")
                props[key.strip()] = value.strip()
    return props


def find_android_sdk():
    sdk = os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME")
    if sdk and os.path.isdir(sdk):
        return sdk
    sdk = read_local_properties().get("sdk.dir")
    if sdk and os.path.isdir(sdk):
        return sdk
    return None


def ndk_version_from_gradle_build():
    """Best-effort: the NDK version the app's Gradle/CMake native build uses.

    Reads app/.cxx/**/android_gradle_build.json and pulls the version out of the
    recorded clang path (…/ndk/<version>/toolchains/…). Matching that NDK keeps
    the clang version aligned with the app and avoids newer NDKs that reject
    flags the xenia CMake still passes (e.g. -Wno-character-conversion)."""
    pattern = os.path.join(self_path, "app", ".cxx", "*", "*", "*",
                           "android_gradle_build.json")
    for path in sorted(glob.glob(pattern), reverse=True):
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception:
            continue
        for tc in data.get("toolchains", {}).values():
            for key in ("cppCompilerExecutable", "cCompilerExecutable"):
                exe = (tc.get(key) or "").replace("\\", "/")
                m = re.search(r"/ndk/([^/]+)/", exe)
                if m:
                    return m.group(1)
    return None


def find_ndk(sdk, override):
    """Returns the NDK path. `override` may be a full path or a version dir."""
    if override:
        if os.path.isdir(override):
            return override
        candidate = os.path.join(sdk, "ndk", override) if sdk else None
        if candidate and os.path.isdir(candidate):
            return candidate
        print_error(f"NDK not found for --ndk={override}")
        sys.exit(1)

    # Prefer the NDK the app's Gradle build uses (see above).
    if sdk:
        gradle_ver = ndk_version_from_gradle_build()
        if gradle_ver:
            candidate = os.path.join(sdk, "ndk", gradle_ver)
            if os.path.isdir(candidate):
                return candidate

    env_ndk = os.environ.get("ANDROID_NDK_HOME") or os.environ.get("ANDROID_NDK_ROOT")
    if env_ndk and os.path.isdir(env_ndk):
        return env_ndk

    if sdk:
        ndk_root = os.path.join(sdk, "ndk")
        if os.path.isdir(ndk_root):
            versions = [d for d in os.listdir(ndk_root)
                        if os.path.isdir(os.path.join(ndk_root, d))]
            if versions:
                def ver_key(v):
                    return tuple(int(x) if x.isdigit() else 0
                                 for x in re.findall(r"\d+", v))
                versions.sort(key=ver_key)
                return os.path.join(ndk_root, versions[-1])
    return None


def find_tool(sdk, name, explicit=None, must=True):
    """Locates a build tool (cmake/ninja/adb) preferring the SDK, then PATH."""
    if explicit:
        if os.path.isfile(explicit):
            return explicit
        found = shutil.which(explicit)
        if found:
            return found
        if must:
            print_error(f"{name} not found at {explicit}")
            sys.exit(1)
        return None

    if shutil.which(name):
        return shutil.which(name)

    if sdk:
        # SDK bundles cmake under cmake/<ver>/bin and adb under platform-tools.
        if name in ("cmake", "ninja", "ctest", "cpack"):
            cmake_root = os.path.join(sdk, "cmake")
            if os.path.isdir(cmake_root):
                bins = []
                for v in os.listdir(cmake_root):
                    exe = os.path.join(cmake_root, v, "bin",
                                       name + (".exe" if os.name == "nt" else ""))
                    if os.path.isfile(exe):
                        bins.append(exe)
                if bins:
                    bins.sort()
                    return bins[-1]
        else:
            exe = os.path.join(sdk, "platform-tools",
                               name + (".exe" if os.name == "nt" else ""))
            if os.path.isfile(exe):
                return exe

    if must:
        print_error(f"{name} not found (install it or pass an explicit path)")
        sys.exit(1)
    return None


def min_sdk_from_gradle():
    """Reads minSdk from app/build.gradle so ANDROID_PLATFORM matches the app."""
    path = os.path.join(self_path, "app", "build.gradle")
    if os.path.isfile(path):
        with open(path, "r", encoding="utf-8") as f:
            m = re.search(r"minSdk\s+(\d+)", f.read())
            if m:
                return int(m.group(1))
    return 28


# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------

def build_bin_dir():
    # The xenia-canary CMake writes runtime outputs to <binary_dir>/bin/Android
    # for single-config (Ninja) Android builds.
    return os.path.join(build_dir, "bin", "Android")


def build_link_shim(args, ndk):
    """Builds libxe_android_shim.a with the NDK clang.

    Returns the archive path (forward slashes) or None if it couldn't be built.
    The same NDK toolchain as the real build is used so the object ABI matches.
    This archive is appended to the test link line (via
    CMAKE_CXX_STANDARD_LIBRARIES) so its members resolve undefined symbols left
    by the xenia static libraries without those libraries being modified. The
    archive bundles:
      * SHIM_SOURCE - stubs for the xe:: system helpers the app supplies;
      * APP_BRIDGE_SOURCE - the one JNI global document_file.cpp needs;
      * APP_BRIDGE_SOURCES - the app-only SAF / DocumentFile / FilePicker
        translation units referenced from code guarded by XE_PLATFORM_AX360E."""
    os.makedirs(shim_dir, exist_ok=True)
    lib_path = os.path.join(shim_dir, "libxe_android_shim.a")

    host = host_tag()
    tool_bin = os.path.join(ndk, "toolchains", "llvm", "prebuilt", host, "bin")
    sysroot = os.path.join(ndk, "toolchains", "llvm", "prebuilt", host, "sysroot")
    exe = ".exe" if os.name == "nt" else ""
    clangxx = os.path.join(tool_bin, "clang++" + exe)
    llvm_ar = os.path.join(tool_bin, "llvm-ar" + exe)
    if not os.path.isfile(clangxx):
        print_error(f"NDK clang++ not found: {clangxx}")
        return None

    arch = ABI_TO_ARCH.get(args.abi, "aarch64")
    triple = {
        "aarch64": f"aarch64-none-linux-android{args.platform}",
        "arm": f"armv7a-none-linux-androideabi{args.platform}",
        "x86_64": f"x86_64-none-linux-android{args.platform}",
        "i686": f"i686-none-linux-android{args.platform}",
    }[arch]

    # Flags shared by every unit compiled into the archive.
    base_flags = [clangxx, f"--target={triple}", f"--sysroot={sysroot}",
                  "-std=gnu++20", "-fPIC", "-ffunction-sections",
                  "-fdata-sections", "-funwind-tables"]

    # Extra include dirs / defines the app translation units need, mirroring
    # the include_directories() the `emu` target uses in app/src/main/cpp.
    app_includes = [
        cpp_dir,
        xenia_canary_dir,
        os.path.join(xenia_canary_dir, "src"),
        os.path.join(xenia_canary_dir, ".."),
    ]
    app_flags = ["-DUSE_CPP17", "-DANDROID"]
    for inc in app_includes:
        app_flags += ["-I", inc.replace("\\", "/")]
    third_party = os.path.join(xenia_canary_dir, "third_party")
    app_flags += ["-isystem", third_party.replace("\\", "/"),
                  "-I", os.path.join(third_party, "fmt", "include").replace("\\", "/")]

    objects = []

    def compile_unit(src_path, obj_path, is_app_src):
        cmd = list(base_flags)
        if is_app_src:
            cmd += app_flags
        cmd += ["-c", src_path.replace("\\", "/"), "-o", obj_path.replace("\\", "/")]
        res = subprocess.run(cmd, cwd=shim_dir, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True)
        if res.returncode != 0:
            print_error("failed to compile " +
                        os.path.basename(src_path) + ":\n" + (res.stdout or ""))
            return False
        objects.append(obj_path.replace("\\", "/"))
        return True

    # 1) The xe:: system stubs.
    stub_cpp = os.path.join(shim_dir, "xe_android_shim.cpp")
    with open(stub_cpp, "w", encoding="utf-8") as f:
        f.write(SHIM_SOURCE)
    if not compile_unit(stub_cpp, os.path.join(shim_dir, "xe_android_shim.o"), False):
        return None

    # 2) The tiny JNI-tree global bridge (document_file.cpp references it).
    bridge_cpp = os.path.join(shim_dir, "xe_app_bridge.cpp")
    with open(bridge_cpp, "w", encoding="utf-8") as f:
        f.write(APP_BRIDGE_SOURCE)
    if not compile_unit(bridge_cpp, os.path.join(shim_dir, "xe_app_bridge.o"), False):
        return None

    # 3) The real app-only sources, cross-compiled from the tree.
    for rel in APP_BRIDGE_SOURCES:
        src = os.path.join(cpp_dir, rel)
        if not os.path.isfile(src):
            print_error(f"app source not found for link shim: {src}")
            return None
        obj = os.path.join(shim_dir, os.path.basename(rel) + ".o")
        if not compile_unit(src, obj, True):
            return None

    ar_bin = llvm_ar if os.path.isfile(llvm_ar) else "ar"
    ar_cmd = [ar_bin, "rcs", lib_path.replace("\\", "/")] + objects
    res = subprocess.run(ar_cmd, cwd=shim_dir, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True)
    if res.returncode != 0:
        print_error("failed to archive link shim:\n" + (res.stdout or ""))
        return None
    return lib_path.replace("\\", "/")


def cmake_configure(args, sdk, ndk, cmake):
    os.makedirs(build_dir, exist_ok=True)
    toolchain = os.path.join(ndk, "build", "cmake", "android.toolchain.cmake")
    if not os.path.isfile(toolchain):
        print_error(f"android toolchain not found: {toolchain}")
        return 1

    build_type = {"debug": "Debug", "release": "Release", "checked": "Debug"}[args.config]

    configure_args = [
        cmake,
        "-S", cpp_dir,
        "-B", build_dir,
        "-G", "Ninja",
        "-DCMAKE_MAKE_PROGRAM=" + args.ninja,
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DANDROID_ABI={args.abi}",
        f"-DANDROID_PLATFORM=android-{args.platform}",
        f"-DANDROID_STL={args.stl}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DXENIA_BUILD_TESTS=ON",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    # Append the Android-only link stubs and the Android log library to the end
    # of every test executable's link line. Being last means the linker pulls
    # them in only to resolve symbols the xenia archives leave undefined.
    std_libs = ""
    if getattr(args, "shim_lib", None):
        std_libs = f"{args.shim_lib} -llog -landroid"
    if std_libs:
        configure_args.append(f"-DCMAKE_CXX_STANDARD_LIBRARIES={std_libs}")

    for define in args.cmake_define:
        configure_args.append(f"-D{define}")

    print_status(f"- configuring Android build ({build_type}, {args.abi}, "
                 f"platform android-{args.platform}, stl {args.stl})...")
    return subprocess.call(configure_args, cwd=cpp_dir)


def cmake_build(args, test_targets):
    print_status(f"- building test targets: {', '.join(test_targets)}...")
    build_args = [args.ninja, "-C", build_dir, *test_targets]
    return subprocess.call(build_args)


# ---------------------------------------------------------------------------
# adb helpers
# ---------------------------------------------------------------------------

def adb_base(args):
    cmd = [args.adb]
    if args.serial:
        cmd += ["-s", args.serial]
    return cmd


def adb_shell(args, remote_cmd, capture=True, timeout=None):
    full = adb_base(args) + ["shell", remote_cmd]
    if capture:
        return subprocess.run(full, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True,
                              timeout=timeout)
    return subprocess.run(full, timeout=timeout)


def check_device(args):
    cmd = adb_base(args) + ["get-state"]
    res = subprocess.run(cmd, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True)
    if res.returncode != 0 or "device" not in (res.stdout or ""):
        print_error(f"no usable adb device (get-state said: "
                    f"{(res.stdout or '').strip()})")
        print("        list devices with `adb devices -l` and pass --serial.")
        return False
    return True


def push_test_binaries(args, binaries):
    adb_shell(args, f"mkdir -p {device_dir}", capture=True)
    for exe in binaries:
        remote = f"{device_dir}/{os.path.basename(exe)}"
        print_status(f"- adb push {os.path.basename(exe)} -> {device_dir}/")
        res = subprocess.run(adb_base(args) + ["push", exe, device_dir + "/"],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True)
        if res.returncode != 0:
            print_error(f"failed to push {os.path.basename(exe)}:\n{res.stdout}")
            return False
        # Ensure the file is executable and lives on a noexec-free partition.
        adb_shell(args, f"chmod 755 {remote}")
    return True


def run_one_test(args, name, extra_args):
    remote = f"{device_dir}/{name}"
    quoted_extra = " ".join(extra_args) if extra_args else ""
    # LD_LIBRARY_PATH lets the binary pick up any pushed .so dependencies.
    remote_cmd = (f"cd {device_dir} && LD_LIBRARY_PATH={device_dir}:$LD_LIBRARY_PATH "
                  f"./{name} {quoted_extra} 2>&1; echo __AX_EXIT__$?")
    start = time.time()
    try:
        res = adb_shell(args, remote_cmd, capture=True, timeout=args.timeout)
    except subprocess.TimeoutExpired:
        return name, None, f"TIMEOUT after {args.timeout}s\n", time.time() - start
    output = res.stdout or ""

    exit_code = None
    m = re.search(r"__AX_EXIT__(\d+)\s*$", output)
    if m:
        exit_code = int(m.group(1))
        output = output[:m.start()]
    return name, exit_code, output, time.time() - start


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def discover_built_binaries(test_targets):
    bin_dir = build_bin_dir()
    found = {}
    missing = []
    for target in test_targets:
        # Android/Ninja produces extension-less executables.
        candidate = os.path.join(bin_dir, target)
        if os.path.isfile(candidate):
            found[target] = candidate
        else:
            missing.append(target)
    return found, missing, bin_dir


def main():
    parser = ArgumentParser(prog="run_test.py",
                            description="Build, push and run xenia Catch2 tests "
                                        "on an Android device via adb.")
    parser.add_argument("--config", choices=["checked", "debug", "release"],
                        default="debug", type=str.lower,
                        help="Build configuration (default: debug).")
    parser.add_argument("--target", action="append", default=[],
                        help="Only build/run the given test target(s). "
                             "Repeatable. Defaults to the standard suites.")
    parser.add_argument("--no_build", action="store_true",
                        help="Skip configure/build; push+run existing binaries.")
    parser.add_argument("--force_reconfigure", action="store_true",
                        help="Delete the CMake cache and configure from scratch.")
    parser.add_argument("--continue", dest="continue_on_error", action="store_true",
                        help="Keep running remaining tests after one fails.")
    parser.add_argument("--serial", default=None,
                        help="adb device serial (required with multiple devices).")
    parser.add_argument("--abi", default="arm64-v8a",
                        help="Android ABI (default: arm64-v8a).")
    parser.add_argument("--platform", type=int, default=None,
                        help="Android API level for ANDROID_PLATFORM "
                             "(default: app minSdk).")
    parser.add_argument("--stl", default="c++_static",
                        help="ANDROID_STL (default: c++_static for a "
                             "self-contained test binary).")
    parser.add_argument("--ndk", default=None,
                        help="NDK path or version subdir under <sdk>/ndk. "
                             "Defaults to the NDK the app's Gradle build uses.")
    parser.add_argument("--cmake", default=None, help="Explicit cmake binary path.")
    parser.add_argument("--ninja", default=None, help="Explicit ninja binary path.")
    parser.add_argument("--adb", default=None, help="Explicit adb binary path.")
    parser.add_argument("--no_shim", action="store_true",
                        help="Do not build/append the Android link shim "
                             "(stub + -llog); tests will likely fail to link.")
    parser.add_argument("--cmake-define", dest="cmake_define", action="append",
                        default=[], metavar="KEY=VALUE",
                        help="Extra -D define for cmake (repeatable).")
    parser.add_argument("--timeout", type=int, default=600,
                        help="Per-test wall-clock timeout in seconds "
                             "(default: 600).")

    # Everything after `--` is forwarded to the test executables.
    argv = sys.argv[1:]
    pass_args = []
    if "--" in argv:
        idx = argv.index("--")
        pass_args = argv[idx + 1:]
        argv = argv[:idx]
    args = parser.parse_args(argv)

    test_targets = args.target or list(DEFAULT_TEST_TARGETS)

    # ---- resolve toolchain ----
    sdk = find_android_sdk()
    if not sdk:
        print_error("Android SDK not found. Set ANDROID_HOME/ANDROID_SDK_ROOT "
                    "or sdk.dir in local.properties.")
        return 1
    print(f"Android SDK: {sdk}")

    args.ninja = find_tool(sdk, "ninja", args.ninja)
    args.cmake = find_tool(sdk, "cmake", args.cmake)
    args.adb = find_tool(sdk, "adb", args.adb)

    ndk = find_ndk(sdk, args.ndk)
    if not ndk:
        print_error("Android NDK not found. Install it or pass --ndk.")
        return 1
    print(f"Android NDK: {ndk}")
    print(f"cmake:  {args.cmake}")
    print(f"ninja:  {args.ninja}")
    print(f"adb:    {args.adb}")

    if args.platform is None:
        args.platform = min_sdk_from_gradle()

    # ---- prepare output dirs ----
    os.makedirs(output_root, exist_ok=True)
    if args.force_reconfigure and os.path.isdir(build_dir):
        shutil.rmtree(build_dir, ignore_errors=True)
    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(logs_dir, exist_ok=True)

    # ---- build ----
    if not args.no_build:
        args.shim_lib = None
        if not args.no_shim:
            print_status("- building Android link shim...")
            args.shim_lib = build_link_shim(args, ndk)
            if args.shim_lib:
                print(f"link shim:  {args.shim_lib}")
            else:
                print_warning("link shim not built; the test link step may fail.")

        ret = cmake_configure(args, sdk, ndk, args.cmake)
        if ret != 0:
            print_error("cmake configure failed.")
            return ret
        ret = cmake_build(args, test_targets)
        if ret != 0:
            print_error("build failed.")
            return ret
    else:
        print_status("- skipping build (--no_build).")

    # ---- locate built binaries ----
    binaries, missing, bin_dir = discover_built_binaries(test_targets)
    if missing:
        for name in missing:
            print_error(f"unable to find built binary for {name} in {bin_dir}"
                        + (" - build it (drop --no_build)." if args.no_build else "."))
        return 1

    # ---- device check ----
    if not check_device(args):
        return 1

    # ---- push ----
    print_status(f"- staging tests on device at {device_dir}")
    if not push_test_binaries(args, list(binaries.values())):
        return 1

    # ---- run ----
    any_failed = False
    summary_lines = []
    summary_lines.append("ax360e Android test run")
    summary_lines.append(f"time:    {time.strftime('%Y-%m-%d %H:%M:%S')}")
    summary_lines.append(f"device:  {args.serial or '(default)'}")
    summary_lines.append(f"config:  {args.config} / {args.abi} / android-{args.platform}")
    summary_lines.append(f"passargs: {' '.join(pass_args)}")
    summary_lines.append("")

    for name in test_targets:
        print(f"\n- {name}")
        test_name, exit_code, output, elapsed = run_one_test(args, name, pass_args)

        log_path = os.path.join(logs_dir, f"{name}.log")
        with open(log_path, "w", encoding="utf-8") as f:
            f.write(f"# {name}\n")
            f.write(f"# exit_code={exit_code} elapsed={elapsed:.2f}s\n\n")
            f.write(output)

        # Mirror the tail to the console for immediate feedback.
        tail = output if len(output) <= 4000 else output[-4000:]
        if len(output) > 4000:
            print("  ... (full output in log)")
        print(tail)

        if exit_code == 0:
            status = "PASS"
            print_status(f"  {name}: PASS ({elapsed:.1f}s)")
        elif exit_code is None:
            status = "TIMEOUT"
            print_error(f"  {name}: {output.strip()}")
            any_failed = True
        else:
            status = f"FAIL (exit={exit_code})"
            print_error(f"  {name}: FAIL exit={exit_code} ({elapsed:.1f}s)")
            any_failed = True

        summary_lines.append(f"{status:<22} {name}  ({elapsed:.1f}s)  -> logs/{name}.log")

        if any_failed and not args.continue_on_error:
            print_error("test failed, aborting (use --continue to keep going).")
            break

    # ---- write summary ----
    summary_lines.append("")
    summary_lines.append("RESULT: " + ("FAILED" if any_failed else "PASSED"))
    summary_text = "\n".join(summary_lines) + "\n"
    with open(os.path.join(output_root, "summary.txt"), "w", encoding="utf-8") as f:
        f.write(summary_text)
    # Echo the one-line result summary to console.
    print("\n" + "\n".join(summary_lines))
    print_status(f"Results written to: {output_root}")

    return 1 if any_failed else 0


if __name__ == "__main__":
    sys.exit(main())
