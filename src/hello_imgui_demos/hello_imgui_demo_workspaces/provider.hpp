#pragma once

#include <functional>

#include "hello_imgui/hello_imgui.h"

#include "view.hpp"

static std::vector<HelloImGui::DockingSplit> GetDefaultDockingSplits(std::string provider_name)
{
    std::vector<HelloImGui::DockingSplit> splits = {
        {provider_name, provider_name + "_CommandSpace", ImGuiDir_Down, 0.5f, 0, ImVec2(100, 100)},
        {provider_name, provider_name + "_LeftSpace", ImGuiDir_Left, 0.25f, 0, ImVec2(100, 100)},
        {provider_name, provider_name + "_RightSpace", ImGuiDir_Right, 0.2f, 0, ImVec2(100, 100)}};

    return splits;
}

class Provider : public ViewDockspace<Provider>
{
   public:
    static constexpr auto DockspaceName = "MainDockSpace";  // parent dockspace to put ourselves in
    static int ctr;

    template<typename... Args>
    Provider(Args&&... args)
        : ViewDockspace<Provider>(std::forward<Args>(args)...)
    {
      if (ViewDockspace<Provider>::dockingParams.dockingSplits.empty())
      {
          ViewDockspace<Provider>::dockingParams.dockingSplits = GetDefaultDockingSplits(ViewDockspace<Provider>::label);
      }
    }

    void RenderDockspaceContent();

    std::vector<std::unique_ptr<JacksView>> jacks;
    std::vector<std::unique_ptr<TimeseriesPlot>> timeseriesPlots;
    std::unique_ptr<SomeModal> modal;
    int m_frameCtr = 0;
};