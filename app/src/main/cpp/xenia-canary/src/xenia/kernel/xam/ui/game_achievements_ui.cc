/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/xam/ui/game_achievements_ui.h"

// For ImGuiContext::NavId (the "nothing focused" indicator used for the
// gamepad B-to-close behavior).
#include "third_party/imgui/imgui_internal.h"

namespace xe {
namespace kernel {
namespace xam {
namespace ui {

GameAchievementsUI::GameAchievementsUI(xe::ui::ImGuiDrawer* imgui_drawer,
                                       const ImVec2 drawing_position,
                                       const TitleInfo* title_info,
                                       const UserProfile* profile)
    : XamDialog(imgui_drawer),
      drawing_position_(drawing_position),
      title_info_(*title_info),
      profile_(profile),
      window_id_(GetWindowId()) {
  LoadAchievementsData();
}

GameAchievementsUI::~GameAchievementsUI() {
  for (auto& entry : achievements_icons_) {
    entry.second.release();
  }
}

bool GameAchievementsUI::LoadAchievementsData() {
  achievements_info_ =
      kernel_state()->xam_state()->achievement_manager()->GetTitleAchievements(
          profile_->xuid(), title_info_.id);

  if (achievements_info_.empty()) {
    return false;
  }

  xe::ui::IconsData data;

  for (const Achievement& entry : achievements_info_) {
    const auto icon =
        kernel_state()->xam_state()->achievement_manager()->GetAchievementIcon(
            profile_->xuid(), title_info_.id, entry.achievement_id);

    data.insert({entry.image_id, icon});
  }

  achievements_icons_ = imgui_drawer()->LoadIcons(data);
  return true;
}

std::string GameAchievementsUI::GetAchievementTitle(
    const Achievement& achievement_entry) const {
  std::string title = "Secret trophy";

  if (achievement_entry.IsUnlocked() || show_locked_info_ ||
      achievement_entry.flags &
          static_cast<uint32_t>(AchievementFlags::kShowUnachieved)) {
    title = xe::to_utf8(achievement_entry.achievement_name);
  }

  return title;
}

std::string GameAchievementsUI::GetAchievementDescription(
    const Achievement& achievement_entry) const {
  std::string description = "Hidden description";

  if (achievement_entry.flags &
      static_cast<uint32_t>(AchievementFlags::kShowUnachieved)) {
    description = xe::to_utf8(achievement_entry.locked_description);
  }

  if (achievement_entry.IsUnlocked() || show_locked_info_) {
    description = xe::to_utf8(achievement_entry.unlocked_description);
  }

  return description;
}

xe::ui::ImmediateTexture* GameAchievementsUI::GetIcon(
    const Achievement& achievement_entry) const {
  if (!achievement_entry.IsUnlocked() && !show_locked_info_) {
    return imgui_drawer()->GetLockedAchievementIcon();
  }

  if (achievements_icons_.count(achievement_entry.image_id)) {
    return achievements_icons_.at(achievement_entry.image_id).get();
  }

  if (achievement_entry.IsUnlocked()) {
    return nullptr;
  }
  return imgui_drawer()->GetLockedAchievementIcon();
}

std::string GameAchievementsUI::GetUnlockedTime(
    const Achievement& achievement_entry) const {
  if (achievement_entry.IsUnlockedOnline()) {
    const auto unlock_time =
        std::chrono::system_clock::to_time_t(chrono::WinSystemClock::to_sys(
            achievement_entry.unlock_time.to_time_point()));

    return fmt::format("Unlocked: Online {:%Y-%m-%d %H:%M}",
                       *std::localtime(&unlock_time));
  }

  if (achievement_entry.unlock_time.is_valid()) {
    const auto unlock_time =
        std::chrono::system_clock::to_time_t(chrono::WinSystemClock::to_sys(
            achievement_entry.unlock_time.to_time_point()));

    return fmt::format("Unlocked: Offline ({:%Y-%m-%d %H:%M})",
                       *std::localtime(&unlock_time));
  }
  return fmt::format("Unlocked: Offline");
}

void GameAchievementsUI::DrawTitleAchievementInfo(
    ImGuiIO& io, const Achievement& achievement_entry) const {
  const auto icon = GetIcon(achievement_entry);

  // One focusable row: the Selectable provides gamepad navigation, focus
  // highlight and auto-scroll; the actual content is drawn over it. The
  // Selectable must be submitted after TableSetColumnIndex(0), otherwise it
  // is drawn outside of any table cell with a broken (zero-width) nav rect.
  ImGui::TableSetColumnIndex(0);
  const float selectable_cursor_y = ImGui::GetCursorPosY();
  ImGui::Selectable("##achievement_row", false,
                    ImGuiSelectableFlags_SpanAllColumns,
                    ImVec2(0.f, xe::ui::default_image_icon_size.y));
  ImGui::SetCursorPosY(selectable_cursor_y);
  if (icon) {
    ImGui::Image(reinterpret_cast<ImTextureID>(icon),
                 xe::ui::default_image_icon_size);
  } else {
    ImGui::Dummy(xe::ui::default_image_icon_size);
  }
  ImGui::TableNextColumn();

  ImGui::PushFont(imgui_drawer()->GetTitleFont());
  const auto primary_line_height = ImGui::GetTextLineHeight();
  ImGui::Text("%s", GetAchievementTitle(achievement_entry).c_str());
  ImGui::PopFont();

  ImGui::PushTextWrapPos(ImGui::GetMainViewport()->Size.x * 0.5f);
  ImGui::TextWrapped("%s",
                     GetAchievementDescription(achievement_entry).c_str());
  ImGui::PopTextWrapPos();

  if (achievement_entry.IsUnlocked()) {
    ImGui::Text("%s", GetUnlockedTime(achievement_entry).c_str());
  }

  ImGui::TableNextColumn();

  // TODO(Gliniak): There is no easy way to align text to middle, so I have to
  // do it manually.
  const float achievement_row_middle_alignment =
      ((xe::ui::default_image_icon_size.x / 2.f) -
       ImGui::GetTextLineHeight() / 2.f) *
      0.85f;

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                       achievement_row_middle_alignment);
  ImGui::PushFont(imgui_drawer()->GetTitleFont());
  ImGui::TextUnformatted(
      fmt::format("{} G", achievement_entry.gamerscore).c_str());
  ImGui::PopFont();
}

void GameAchievementsUI::OnDraw(ImGuiIO& io) {
  ImGui::SetNextWindowPos(drawing_position_, ImGuiCond_FirstUseEver);

  const auto xenia_window_size = ImGui::GetMainViewport()->Size;

  ImGui::SetNextWindowSizeConstraints(
      ImVec2(xenia_window_size.x * 0.2f, xenia_window_size.y * 0.3f),
      ImVec2(xenia_window_size.x * 0.6f, xenia_window_size.y * 0.8f));
  ImGui::SetNextWindowBgAlpha(0.8f);

  bool dialog_open = true;

  std::string title_name = xe::to_utf8(title_info_.title_name);
  title_name.erase(std::ranges::remove(title_name, '\0').begin(),
                   title_name.end());

  const std::string window_name =
      fmt::format("{} Achievements###{}", title_name, window_id_);
  if (!ImGui::Begin(window_name.c_str(), &dialog_open,
                    ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    Close();
    ImGui::End();
    return;
  }

  // Gamepad B with nothing focused closes the window. With an item focused,
  // B first backs out of the focus (standard ImGui behavior) - that press
  // clears g.NavId inside NewFrame, before this code runs, so the press that
  // just exited the focus is recognized by prev_nav_id_ still being set.
  // Note: io.NavActive stays true the whole time the window is focused, so
  // NavId == 0 is the proper "nothing focused" indicator here.
  const ImGuiID current_nav_id = ImGui::GetCurrentContext()->NavId;
  if (prev_nav_id_ == 0 && current_nav_id == 0 &&
      ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false)) {
    dialog_open = false;
  }
  prev_nav_id_ = current_nav_id;

  ImGui::Checkbox("Show locked achievements information", &show_locked_info_);
  ImGui::Separator();

  if (achievements_info_.empty()) {
    ImGui::TextUnformatted(fmt::format("No achievements data!").c_str());
  } else {
    // The window itself scrolls (clamped by the size constraints); the
    // gamepad navigation auto-scrolls to the focused row.
    if (ImGui::BeginTable("", 3, ImGuiTableFlags_BordersInnerH)) {
      uint32_t row_index = 0;
      for (const auto& entry : achievements_info_) {
        // Unique ID per row - every row uses the same Selectable label.
        ImGui::PushID(static_cast<int>(row_index++));
        ImGui::TableNextRow(0, xe::ui::default_image_icon_size.y);
        DrawTitleAchievementInfo(io, entry);
        ImGui::PopID();
      }

      ImGui::EndTable();
    }
  }

  if (!dialog_open) {
    Close();
    ImGui::End();
    return;
  }

  ImGui::End();
};

}  // namespace ui
}  // namespace xam
}  // namespace kernel
}  // namespace xe
