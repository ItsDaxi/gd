#pragma once
#include <imgui.h>

namespace mh::gui {

void applyTheme();
void applyPreset(int preset);
int presetCount();

ImU32 accentU32(float alpha = 1.0f);
ImVec4 accent();
ImVec4 titleAccent();
ImVec4 textColor();
ImVec4 dimText();
ImVec4 rowColor();
ImVec4 rowHover();

} // namespace mh::gui
