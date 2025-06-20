
#pragma once

#include "imgui.h"

#include <map>
#include <string>
#include <vector>

struct DockableWindowWaitingForAddition;

struct HelloImGuiContext
{
    bool Initialized;

    std::vector<DockableWindowWaitingForAddition> DockableWindowsToAdd;
    std::vector<std::string> DockableWindowsToRemove;

    std::map<std::string, ImGuiID> ImGuiSplitIDs;
};

#ifndef GHelloImGui
extern HelloImGuiContext* GHelloImGui;  // Current implicit context pointer
#endif
