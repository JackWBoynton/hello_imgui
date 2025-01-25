#pragma once

#include "hello_imgui/hello_imgui.h"

#include "view.hpp"

class ProviderSelectorView : public View<ProviderSelectorView> {
  public:
    ProviderSelectorView(std::string dockspace_name) : View<ProviderSelectorView>({"Provider Selector", dockspace_name}) {}

    void Render();
};