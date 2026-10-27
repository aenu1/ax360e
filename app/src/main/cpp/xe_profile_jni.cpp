// SPDX-License-Identifier: WTFPL
// Xbox 360 用户档案 (Profile) 的独立管理桥: 不依赖运行中的 KernelState,
// 直接在 content_root 上复用 xenia 的 ProfileManager 静态加解密与目录布局。
// 磁盘布局 (与 ProfileManager::GetProfilePath 一致):
//   content/<XUID16>/FFFE07D1/00010000/<XUID16>/Account   加密的 X_XAMACCOUNTINFO
//   content/<XUID16>/FFFE07D1/Headers/00010000/<XUID16>.header  包元数据
#include <jni.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <regex>
#include <string>
#include <vector>

#include "third_party/fmt/include/fmt/format.h"
#include "xenia/base/math.h"
#include "xenia/base/string.h"
#include "xenia/base/string_util.h"
#include "xenia/kernel/title_id_utils.h"
#include "xenia/kernel/xam/profile_manager.h"
#include "xenia/kernel/xam/xcontent/xcontent_package_directory.h"
#include "xenia/vfs/virtual_file_system.h"
#include "xenia/xbox.h"

using xe::kernel::xam::ProfileManager;
using xe::kernel::xam::XCONTENT_DATA_INTERNAL;

static const std::regex& xuid_pattern() {
  static const std::regex re("^[0-9A-F]{16}$");
  return re;
}

// "User_<XUID>:\Account" 对应的宿主路径 (目录包)
static std::filesystem::path package_path_of(
    const std::filesystem::path& content_root, const std::string& xuid) {
  return content_root / xuid / "FFFE07D1" / "00010000" / xuid;
}

static bool read_account(const std::filesystem::path& package_path,
                         xe::kernel::xam::X_XAMACCOUNTINFO* out) {
  std::ifstream in(package_path / "Account", std::ios::binary);
  if (!in) return false;

  std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
  if (data.size() < sizeof(xe::kernel::xam::X_XAMACCOUNTINFO) + 0x18)
    return false;

  // 与 ProfileManager::LoadAccount 相同: 先 retail key, 失败再试 devkit
  return ProfileManager::DecryptAccountFile(data.data(), out) ||
         ProfileManager::DecryptAccountFile(data.data(), out, true);
}

static bool write_account(const std::filesystem::path& package_path,
                          const xe::kernel::xam::X_XAMACCOUNTINFO* acct) {
  std::vector<uint8_t> data(sizeof(xe::kernel::xam::X_XAMACCOUNTINFO) + 0x18);
  ProfileManager::EncryptAccountFile(acct, data.data());

  FILE* f =
      std::fopen((package_path / "Account").c_str(), "wb");
  if (!f) return false;
  size_t written = std::fwrite(data.data(), 1, data.size(), f);
  std::fclose(f);
  return written == data.size();
}

// 与 ProfileManager::CreateProfile 相同的包创建流程 (目录包 + Headers 元数据)
static bool create_package(const std::filesystem::path& content_root,
                           const std::string& xuid) {
  const auto package_path = package_path_of(content_root, xuid);
  std::error_code ec;
  if (!std::filesystem::create_directories(package_path, ec)) return false;

  XCONTENT_DATA_INTERNAL data{};
  data.title_id = xe::kernel::kDashboardID;
  data.content_type = xe::XContentType::kProfile;
  data.xuid = xe::string_util::from_string<uint64_t>(xuid, true);
  data.set_file_name(xuid);

  // 构造即在析构前写出 Headers 元数据文件 (WriteContentHeaderFile)
  xe::vfs::VirtualFileSystem fs;  // 仅作指针传递, 包不挂载
  xe::kernel::xam::ContentPackageDirectory package(&fs, "", package_path, data,
                                                   nullptr, nullptr);
  return true;
}

// ------------------------- JNI -------------------------

static std::string jstr_to_std(JNIEnv* env, jstring s) {
  if (!s) return {};
  const char* c = env->GetStringUTFChars(s, nullptr);
  std::string out(c);
  env->ReleaseStringUTFChars(s, c);
  return out;
}

// public native String xe_profile_list(String content_root)
// 返回多行 "XUID16<TAB>Gamertag"; 解密失败的档案 gamertag 为空串
static jstring j_xe_profile_list(JNIEnv* env, jobject self,
                                 jstring content_root_str) {
  const auto content_root =
      std::filesystem::path(jstr_to_std(env, content_root_str));
  std::string result;

  std::error_code ec;
  for (const auto& entry :
       std::filesystem::directory_iterator(content_root, ec)) {
    const std::string name = xe::path_to_utf8(entry.path().filename());
    if (!std::regex_match(name, xuid_pattern()) || name == "0000000000000000")
      continue;
    if (!std::filesystem::exists(package_path_of(content_root, name)))
      continue;

    xe::kernel::xam::X_XAMACCOUNTINFO acct;
    std::string gamertag;
    if (read_account(package_path_of(content_root, name), &acct))
      gamertag = acct.GetGamertagString();

    result += name;
    result += '\t';
    result += gamertag;
    result += '\n';
  }
  return env->NewStringUTF(result.c_str());
}

// public native boolean xe_profile_create(String content_root, String gamertag)
static jboolean j_xe_profile_create(JNIEnv* env, jobject self,
                                    jstring content_root_str,
                                    jstring gamertag_str) {
  const auto content_root =
      std::filesystem::path(jstr_to_std(env, content_root_str));
  const std::string gamertag = jstr_to_std(env, gamertag_str);
  if (!ProfileManager::IsGamertagValid(gamertag)) return JNI_FALSE;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::string xuid;
  for (int i = 0; i < 32; i++) {
    const uint64_t candidate =
        ((uint64_t)0xE03 << 52) + (gen() % (1 << 31));
    xuid = fmt::format("{:016X}", candidate);
    if (!std::filesystem::exists(content_root / xuid)) break;
  }

  const auto package_path = package_path_of(content_root, xuid);
  if (!create_package(content_root, xuid)) return JNI_FALSE;

  xe::kernel::xam::X_XAMACCOUNTINFO acct = {};
  const std::u16string gamertag_u16 = xe::to_utf16(gamertag);
  xe::string_util::copy_and_swap_truncating(
      acct.gamertag, gamertag_u16, xe::countof(acct.gamertag));
  return write_account(package_path, &acct) ? JNI_TRUE : JNI_FALSE;
}

// public native boolean xe_profile_rename(String content_root, String xuid, String gamertag)
static jboolean j_xe_profile_rename(JNIEnv* env, jobject self,
                                    jstring content_root_str, jstring xuid_str,
                                    jstring gamertag_str) {
  const auto content_root =
      std::filesystem::path(jstr_to_std(env, content_root_str));
  const std::string xuid = jstr_to_std(env, xuid_str);
  const std::string gamertag = jstr_to_std(env, gamertag_str);
  if (!std::regex_match(xuid, xuid_pattern())) return JNI_FALSE;
  if (!ProfileManager::IsGamertagValid(gamertag)) return JNI_FALSE;

  const auto package_path = package_path_of(content_root, xuid);
  xe::kernel::xam::X_XAMACCOUNTINFO acct;
  if (!read_account(package_path, &acct)) return JNI_FALSE;

  const std::u16string gamertag_u16 = xe::to_utf16(gamertag);
  xe::string_util::copy_and_swap_truncating(
      acct.gamertag, gamertag_u16, xe::countof(acct.gamertag));
  return write_account(package_path, &acct) ? JNI_TRUE : JNI_FALSE;
}

// public native boolean xe_profile_delete(String content_root, String xuid)
static jboolean j_xe_profile_delete(JNIEnv* env, jobject self,
                                    jstring content_root_str,
                                    jstring xuid_str) {
  const auto content_root =
      std::filesystem::path(jstr_to_std(env, content_root_str));
  const std::string xuid = jstr_to_std(env, xuid_str);
  if (!std::regex_match(xuid, xuid_pattern())) return JNI_FALSE;

  std::error_code ec;
  std::filesystem::remove_all(content_root / xuid, ec);
  return !ec ? JNI_TRUE : JNI_FALSE;
}

int register_ax360e_profile(JNIEnv* env) {
  static const JNINativeMethod methods[] = {
      {"xe_profile_list",
       "(Ljava/lang/String;)Ljava/lang/String;",
       (void*)j_xe_profile_list},
      {"xe_profile_create",
       "(Ljava/lang/String;Ljava/lang/String;)Z",
       (void*)j_xe_profile_create},
      {"xe_profile_rename",
       "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z",
       (void*)j_xe_profile_rename},
      {"xe_profile_delete",
       "(Ljava/lang/String;Ljava/lang/String;)Z",
       (void*)j_xe_profile_delete},
  };
  jclass clazz = env->FindClass("aenu/ax360e/Emulator");
  if (!clazz) return JNI_ERR;
  return env->RegisterNatives(clazz, methods,
                              sizeof(methods) / sizeof(methods[0]));
}
