#pragma once
#include "imgui.h"
#include "hello_imgui/docking_params.h"
#include "hello_imgui/runner_params.h"
#include "hello_imgui/imgui_window_params.h"
#include <functional>
#include <memory>

namespace HelloImGui
{

// Internal functions below
namespace DockingDetails
{
void ShowToolbars(const RunnerParams& runnerParams);
void ConfigureImGuiDocking(const ImGuiWindowParams& imGuiWindowParams);
void ProvideWindowOrDock(RunnerParams& runnerParams);
void CloseWindowOrDock(ImGuiWindowParams& imGuiWindowParams);
void ShowViewMenu(RunnerParams & runnerParams);
void ShowDockableWindows(std::vector<DockableWindow*>& dockableWindows);
bool ApplyWindowDockingLocation(DockableWindow* dockableWindow);
}  // namespace DockingDetails

}  // namespace HelloImGui