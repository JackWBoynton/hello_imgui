#pragma once

#include <concepts>

#include "hello_imgui/dpi_aware.h"
#include "hello_imgui/hello_imgui.h"

class NamedInstance
{
    // helper class that provides a unique name for each instance
    static std::unordered_map<std::string, int> m_instanceCount;

   public:
    NamedInstance(const std::string& base_type) : m_base_type(base_type) {}

    std::string GetQualifiedName()
    {
        if (m_instanceCount.find(m_base_type) == m_instanceCount.end())
            m_instanceCount[m_base_type] = 0;

        if (!m_assigned_name.empty())
            return m_assigned_name;

        m_instanceCount[m_base_type]++;

        // zero indexing
        m_assigned_name = m_instanceCount[m_base_type] > 1
                              ? m_base_type + std::to_string(m_instanceCount[m_base_type] - 1)
                              : m_base_type;
        return m_assigned_name;
    }

    std::string GetBaseType() { return m_base_type; }
    std::string GetAssignedName() { return m_assigned_name; }

   private:
    std::string m_base_type;
    std::string m_assigned_name;
};

// the biggest assumption here is that ALL Labels are unique
template <typename Derived>
struct View : public NamedInstance, public HelloImGui::DockableWindow
{
    View(HelloImGui::DockableWindow dockableWindow)
        : NamedInstance(dockableWindow.label), HelloImGui::DockableWindow(dockableWindow)
    {
        if (GuiFunction == nullptr)
        {
            GuiFunction = [this] { static_cast<Derived*>(this)->Render(); };
        }

        HelloImGui::DockableWindow::label = NamedInstance::GetQualifiedName();
        HelloImGui::AddDockableWindow(this);
    }

    ~View() { HelloImGui::RemoveDockableWindow(label); }
};

template <typename Derived>
class Modal : public View<Derived>
{
   public:
    template <typename... Args>
    Modal(Args&&... args) : View<Derived>({std::forward<Args>(args)...})
    {
        HelloImGui::DockableWindow::callBeginEnd = false;
    }

    void Render()
    {
        if (ImGui::BeginPopupModal(View<Derived>::label.c_str()))
        {
            static_cast<Derived*>(this)->RenderModalContent();
            ImGui::EndPopup();
        }
    }
};

class JacksView : public View<JacksView>
{
   public:
    JacksView(std::string DockspaceName) : View<JacksView>({"Jack", DockspaceName}) {
        memset(bools, 0, sizeof(bools));
    }

    void Render() {
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                ImGui::Checkbox(("##" + std::to_string(i) + ":" + std::to_string(j)).c_str(), &bools[i][j]);
                ImGui::SameLine();
            }

            ImGui::NewLine();
        }
     }

     private:
        bool bools[10][10];
};

namespace views::helpers
{
void RenderDockableWindowViews(std::vector<HelloImGui::DockableWindow*>& dockableWindows);
void OverrideWindowFlags(std::vector<HelloImGui::DockableWindow*>& dockableWindows,
                         ImGuiWindowFlags flagsToOverride,
                         bool enable);
void SetWindowCanClose(std::vector<HelloImGui::DockableWindow*>& dockableWindows, bool canClose);
}  // namespace views::helpers

typedef int ViewDockspaceOptions;

enum ViewDockspaceOptions_
{
    ViewDockspaceOptions_None = 0,
    ViewDockspaceOptions_NoViewMenu = 1 << 1,
    ViewDockspaceOptions_NoFileMenu = 1 << 2,
    ViewDockspaceOptions_NoLayoutLockable = 1 << 3,
    ViewDockspaceOptions_NoRender = 1 << 4,
};

template <typename Derived>
struct ViewDockspace : public View<Derived>
{
    template <typename... Args>
    ViewDockspace(Args&&... args)
        : View<Derived>({std::forward<Args>(args)...}),
          m_viewDockspaceOptions(ViewDockspaceOptions_None)
    {
        if (View<Derived>::GuiFunction == nullptr)
        {
            View<Derived>::GuiFunction = [this] { this->Render(); };
        }
    } 

    template <typename... Args>
    ViewDockspace(ViewDockspaceOptions viewDockspaceOptions, Args&&... args)
        : View<Derived>({std::forward<Args>(args)...}),
          m_viewDockspaceOptions(viewDockspaceOptions)
    {
        if (View<Derived>::GuiFunction == nullptr && !(m_viewDockspaceOptions & ViewDockspaceOptions_NoRender))
        {
            View<Derived>::GuiFunction = [this] { this->Render(); };
        }
    }

    void Render()
    {
        if ((HelloImGui::DockableWindow::imGuiWindowFlags & ImGuiWindowFlags_MenuBar) &&
            ImGui::BeginMenuBar())
        {
            if (!(m_viewDockspaceOptions & ViewDockspaceOptions_NoFileMenu) && ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Remove Provider"))
                {
                    HelloImGui::DockableWindow::isVisible = false;
                    HelloImGui::RemoveDockableWindow(HelloImGui::DockableWindow::label);
                    m_shouldRemove = true;
                }
                ImGui::EndMenu();
            }

            if (!(m_viewDockspaceOptions & ViewDockspaceOptions_NoViewMenu) && ImGui::BeginMenu("View"))
            {
                views::helpers::RenderDockableWindowViews(
                    HelloImGui::DockableWindow::dockingParams.dockableWindows);
                ImGui::EndMenu();
            }

            if (!(m_viewDockspaceOptions & ViewDockspaceOptions_NoLayoutLockable) &&
                ImGui::BeginMenu("Layout"))
            {
                // if we set the layout to be locked, recursively set all the children to be locked using
                // ApplyWindowFlagDockableWindowViews make this a toggle selector that applies and removes
                // m_subWindowFlags
                if (ImGui::MenuItem("Lock Layout", NULL, m_subWindowFlags & ImGuiWindowFlags_NoMove))
                {
                    views::helpers::OverrideWindowFlags(
                        HelloImGui::DockableWindow::dockingParams.dockableWindows,
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse,
                        !(m_subWindowFlags & ImGuiWindowFlags_NoMove));
                    views::helpers::SetWindowCanClose(
                        HelloImGui::DockableWindow::dockingParams.dockableWindows,
                        (m_subWindowFlags & ImGuiWindowFlags_NoMove));
                    m_subWindowFlags ^= ImGuiWindowFlags_NoMove;
                    HelloImGui::DockableWindow::dockingParams.mainDockSpaceNodeFlags ^=
                        ImGuiDockNodeFlags_NoResize;
                }

                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Text(
                        "Locks the layout in place, preventing the user from moving or resizing windows.");
                    ImGui::Text("This is useful for preventing accidental layout changes.");
                    ImGui::Text(
                        "This will not affect any windows that are added after the layout is locked, so you "
                        "will need to toggle to apply to new windows.");
                    ImGui::EndTooltip();
                }

                ImGui::BeginDisabled(m_subWindowFlags & ImGuiWindowFlags_NoMove);
                if (ImGui::MenuItem("Reset Layout"))
                {
                    HelloImGui::DockableWindow::dockingParams.layoutReset = true;
                }
                ImGui::EndDisabled();

                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Resets the layout to the default docking layout.");
                    if (m_subWindowFlags & ImGuiWindowFlags_NoMove)
                    {
                        ImGui::Text("This is not possible while the layout is locked.");
                    }
                    ImGui::EndTooltip();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        if (!(m_viewDockspaceOptions & ViewDockspaceOptions_NoRender))
        {
            static_cast<Derived*>(this)->RenderDockspaceContent();
        }
    }

    bool GetShouldRemove() const { return m_shouldRemove; }

   private:
    const ViewDockspaceOptions m_viewDockspaceOptions;
    bool m_shouldRemove = false;

    ImGuiWindowFlags m_subWindowFlags = ImGuiWindowFlags_None;
};

class TimeseriesPlot : public View<TimeseriesPlot>
{
   public:
    TimeseriesPlot(const std::string& dockSpaceName) : View<TimeseriesPlot>({"TimeSeriesPlot", dockSpaceName})
    {
    }

    void Render()
    {
        ImGui::Text("TimeseriesPlot");
        ImGui::PlotLines(
            "Line",
            [](void* data, int idx) { return 50.f + 50.f * sinf(0.1f * idx); },
            NULL,
            100,
            0,
            NULL,
            FLT_MAX,
            FLT_MAX,
            HelloImGui::EmToVec2(0, 80));
    }
};

class SomeModal : public Modal<SomeModal>
{
   public:
    SomeModal(const std::string& dockSpaceName) : Modal<SomeModal>("SomeModal", dockSpaceName)
    {
    }

    void RenderModalContent()
    {
        ImGui::Text("SomeModal");
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
    }
};