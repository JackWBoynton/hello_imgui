#pragma once
#include "hello_imgui/docking_params.h"
#include "hello_imgui/imgui_window_params.h"
#include "hello_imgui/runner_params.h"
#include "imgui.h"
#include <functional>

struct DockableWindowWaitingForAddition
{
    std::shared_ptr<HelloImGui::DockableWindow> dockableWindow;
    bool forceDockspace;
    uint8_t waitingFramesBeforeAdd = 10;
};

namespace HelloImGui
{

// Used for external rendering of dockable windows
// this works because ImGui::Begin(some_id) can be called multiple times per frame for the same ID so
// HelloImGui can maintain layouts/docking while the user can call Begin/End to render the window contents
// separately. Begin(...) will return false if the window is collapsed/hidden so the user can skip rendering.
bool Begin(const std::shared_ptr<DockableWindow>& dockableWindow);
void End();

// Internal functions below
namespace DockingDetails
{
    void ShowToolbars(const RunnerParams& runnerParams);
    void ConfigureImGuiDocking(const ImGuiWindowParams& imGuiWindowParams);
    void ProvideWindowOrDock(RunnerParams& runnerParams);
    void CloseWindowOrDock(ImGuiWindowParams& imGuiWindowParams);
    void ShowViewMenu(RunnerParams& runnerParams);
    void ShowDockableWindows(std::vector<std::shared_ptr<DockableWindow>>& dockableWindows);
    void RenderDockableWindowViews(std::vector<std::shared_ptr<DockableWindow>>& dockableWindows);
}  // namespace DockingDetails

}  // namespace HelloImGui

namespace ImGui
{

auto UpdateStringSizeCallback(ImGuiInputTextCallbackData* data) -> int;

auto InputText(char const* label, std::string& buffer, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
    -> bool;

auto InputText(char const* label, std::u8string& buffer, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
    -> bool;

auto InputTextMultiline(char const* label,
                        std::string& buffer,
                        ImVec2 const& size,
                        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None) -> bool;

auto InputTextWithHint(char const* label,
                       char const* hint,
                       std::string& buffer,
                       ImGuiInputTextFlags flags = ImGuiInputTextFlags_None) -> bool;
}  // namespace ImGui
