#pragma once
#include "hello_imgui/docking_params.h"
#include "hello_imgui/imgui_window_params.h"
#include "hello_imgui/runner_params.h"
#include "imgui.h"
#include <functional>

namespace HelloImGui
{

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
