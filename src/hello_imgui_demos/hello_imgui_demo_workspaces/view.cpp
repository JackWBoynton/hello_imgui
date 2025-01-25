
#include <imgui.h>
#include <imgui_internal.h>

#include "view.hpp"

namespace views::helpers {

void RenderDockableWindowViews(std::vector<HelloImGui::DockableWindow*>& dockableWindows)
{
    for (auto& dockableWindow : dockableWindows)
    {
        if (!dockableWindow->includeInViewMenu)
            continue;

        if (!dockableWindow->dockingParams.dockableWindows.empty() && !ImGui::IsKeyDown(ImGuiKey_ModShift) &&
            dockableWindow->isVisible)
        {
            if (ImGui::BeginMenu(dockableWindow->label.c_str()))
            {
                RenderDockableWindowViews(dockableWindow->dockingParams.dockableWindows);
                ImGui::EndMenu();
            }
        }
        else
        {
            if (dockableWindow->canBeClosed)
            {
                if (ImGui::MenuItem(dockableWindow->label.c_str(), nullptr, dockableWindow->isVisible))
                    dockableWindow->isVisible = !dockableWindow->isVisible;
            }
            else
            {
                ImGui::MenuItem(dockableWindow->label.c_str(), nullptr, dockableWindow->isVisible, false);
            }
        }
    }
}

static std::map<int, ImGuiWindowFlags> s_flagBackup;

void OverrideWindowFlags(
    std::vector<HelloImGui::DockableWindow*>& dockableWindows, 
    ImGuiWindowFlags flagsToOverride, 
    bool enable)
{
    ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();

    for (auto& dockableWindow : dockableWindows)
    {
        // If this "window" has nested dockable windows, recurse
        if (!dockableWindow->dockingParams.dockableWindows.empty())
        {
            OverrideWindowFlags(dockableWindow->dockingParams.dockableWindows, flagsToOverride, enable);
            continue;
        }

        // Get a unique ID for the window based on its label
        ImGuiID windowId = currentWindow->GetID(dockableWindow->label.c_str());

        if (enable)
        {
            // Before we set new flags, store the old ones if not already stored
            if (s_flagBackup.find(windowId) == s_flagBackup.end())
            {
                s_flagBackup[windowId] = dockableWindow->imGuiWindowFlags;
            }

            // Add the desired flags
            dockableWindow->imGuiWindowFlags |= flagsToOverride;
        }
        else
        {
            // Revert to the old flags if we have them
            if (auto it = s_flagBackup.find(windowId); it != s_flagBackup.end())
            {
                dockableWindow->imGuiWindowFlags = it->second;
                // If desired, remove from the backup map once reverted
                s_flagBackup.erase(it);
            }
        }
    }
}

static std::map<ImGuiID, bool> s_canCloseBackup;

void SetWindowCanClose(
    std::vector<HelloImGui::DockableWindow*>& dockableWindows, 
    bool canClose)
{
    ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();

    for (auto& dockableWindow : dockableWindows)
    {
        // If this "window" has nested dockable windows, recurse
        if (!dockableWindow->dockingParams.dockableWindows.empty())
        {
            SetWindowCanClose(dockableWindow->dockingParams.dockableWindows, canClose);
            continue;
        }

        // Get a unique ID for the window based on its label
        ImGuiID windowId = currentWindow->GetID(dockableWindow->label.c_str());

        if (!canClose)
        {
            // We are overriding to force "not closable":
            // 1) store the old value if not already stored
            // 2) set canBeClosed = false
            if (s_canCloseBackup.find(windowId) == s_canCloseBackup.end())
            {
                s_canCloseBackup[windowId] = dockableWindow->canBeClosed;
            }
            dockableWindow->canBeClosed = false;
        }
        else
        {
            // We are reverting to the old canBeClosed value
            if (auto it = s_canCloseBackup.find(windowId); it != s_canCloseBackup.end())
            {
                dockableWindow->canBeClosed = it->second;
                s_canCloseBackup.erase(it);  // optional cleanup
            }
        }
    }
}

}