/*
A more complex app demo

It demonstrates how to:
- set up a complex docking layouts (with several possible layouts):
- use the status bar
- use default menus (App and view menu), and how to customize them
- display a log window
- load additional fonts, possibly colored, and with emojis
- use a specific application state (instead of using static variables)
- save some additional user settings within imgui ini file
- use borderless windows, that are movable and resizable
*/

#include "entt.hpp"
#include "hello_imgui/hello_imgui.h"
#include "hello_imgui/icons_font_awesome_6.h"
#include "hello_imgui/renderer_backend_options.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "nlohmann/json.hpp"
#include <implot.h>
#include <random>

using entt::literals::operator""_hs;

#include <tracy/Tracy.hpp>

#include <functional>
#include <sstream>

// Poor man's fix for C++ late arrival in the unicode party:
//    - C++17: u8"my string" is of type const char*
//    - C++20: u8"my string" is of type const char8_t*
// However, ImGui text functions expect const char*.
#ifdef __cpp_char8_t
#define U8_TO_CHAR(x) reinterpret_cast<const char*>(x)
#else
#define U8_TO_CHAR(x) x
#endif
// And then, we need to tell gcc to stop validating format string (it gets confused by the u8"" string)
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat"
#endif

#include <cstddef>
#include <cstdlib>
#include <new>

// =====================
// Tracy config
// =====================
static constexpr int kCsDepth = 16;

// =====================
// helpers
// =====================
static inline void* alloc_or_throw(std::size_t sz)
{
    if (void* p = std::malloc(sz))
        return p;
    throw std::bad_alloc();
}

static inline void free_ptr(void* p) noexcept { std::free(p); }

// =====================
// scalar new / delete
// =====================
void* operator new(std::size_t sz)
{
    void* p = alloc_or_throw(sz);
    TracyAllocS(p, sz, kCsDepth);
    return p;
}

void operator delete(void* p) noexcept
{
    if (!p)
        return;
    TracyFreeS(p, kCsDepth);
    free_ptr(p);
}

// sized delete (C++14+)
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }

// =====================
// array new / delete
// =====================
void* operator new[](std::size_t sz)
{
    void* p = alloc_or_throw(sz);
    TracyAllocS(p, sz, kCsDepth);
    return p;
}

void operator delete[](void* p) noexcept
{
    if (!p)
        return;
    TracyFreeS(p, kCsDepth);
    free_ptr(p);
}

void operator delete[](void* p, std::size_t) noexcept { operator delete[](p); }

// =====================
// nothrow variants
// =====================
void* operator new(std::size_t sz, const std::nothrow_t&) noexcept
{
    if (void* p = std::malloc(sz))
    {
        TracyAllocS(p, sz, kCsDepth);
        return p;
    }
    return nullptr;
}

void operator delete(void* p, const std::nothrow_t&) noexcept { operator delete(p); }

void* operator new[](std::size_t sz, const std::nothrow_t&) noexcept
{
    if (void* p = std::malloc(sz))
    {
        TracyAllocS(p, sz, kCsDepth);
        return p;
    }
    return nullptr;
}

void operator delete[](void* p, const std::nothrow_t&) noexcept { operator delete[](p); }

// =====================
// aligned new / delete (C++17+)
// =====================
#if __cpp_aligned_new

static inline void* aligned_alloc_or_throw(std::size_t sz, std::size_t align)
{
#if defined(_MSC_VER)
    void* p = _aligned_malloc(sz, align);
    if (!p)
        throw std::bad_alloc();
    return p;
#else
    // aligned_alloc requires size multiple of alignment
    std::size_t rounded = (sz + align - 1) & ~(align - 1);
    void* p = std::aligned_alloc(align, rounded);
    if (!p)
        throw std::bad_alloc();
    return p;
#endif
}

static inline void aligned_free_ptr(void* p) noexcept
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

void* operator new(std::size_t sz, std::align_val_t al)
{
    std::size_t align = static_cast<std::size_t>(al);
    void* p = aligned_alloc_or_throw(sz, align);
    TracyAllocS(p, sz, kCsDepth);
    return p;
}

void operator delete(void* p, std::align_val_t) noexcept
{
    if (!p)
        return;
    TracyFreeS(p, kCsDepth);
    aligned_free_ptr(p);
}

void operator delete(void* p, std::size_t, std::align_val_t al) noexcept { operator delete(p, al); }

void* operator new[](std::size_t sz, std::align_val_t al)
{
    std::size_t align = static_cast<std::size_t>(al);
    void* p = aligned_alloc_or_throw(sz, align);
    TracyAllocS(p, sz, kCsDepth);
    return p;
}

void operator delete[](void* p, std::align_val_t) noexcept
{
    if (!p)
        return;
    TracyFreeS(p, kCsDepth);
    aligned_free_ptr(p);
}

void operator delete[](void* p, std::size_t, std::align_val_t al) noexcept { operator delete[](p, al); }

#endif  // __cpp_aligned_new

using Registry = entt::registry;
using Entity = entt::entity;
using Handle = entt::handle;

template <typename T>
struct RenderableTrait
{
    static void Render(Handle, T&) {}
    static void RenderProperties(Handle, T&) {}

    static_assert(sizeof(T) == -1, "RenderableTrait not specialized for this type");
};

template <typename T>
struct SerializableTrait
{
    static nlohmann::json Serialize(const T&) { return {}; }
    static void Deserialize(const nlohmann::json&, T&) {}

    static_assert(sizeof(T) == -1, "SerializableTrait not specialized for this type");
};

struct DockableWindowHandle
{
    std::shared_ptr<HelloImGui::DockableWindow> Window;

    DockableWindowHandle(std::shared_ptr<HelloImGui::DockableWindow> window) : Window(window)
    {
        HelloImGui::AddDockableWindow(window);
    }

    ~DockableWindowHandle() { HelloImGui::RemoveDockableWindow(Window->label); }
};

template <typename T>
struct Archetype
{
    static constexpr auto Identifier = "HelloImGui.ecs_probe.UnknownArchetype";
    static constexpr auto Version = 1;
    static constexpr auto InstanceName = "Unknown Archetype";

    static_assert(sizeof(T) == -1, "Archetype not specialized for this type");
};

struct JacksView
{
    std::string Message;
    int Type = rand() % 3;

    std::array<float, 100> DataX;
    std::array<float, 100> DataY;

    JacksView()
    {
        switch (Type)
        {
            case 0:
                Message = "Jack Type A";
                break;
            case 1:
                Message = "Jack Type B";
                break;
            case 2:
                Message = "Jack Type C";
                break;
            default:
                Message = "Unknown Jack Type";
                break;
        }
    }
};

template <>
struct Archetype<JacksView>
{
    static constexpr auto Identifier = "HelloImGui.ecs_probe.JacksView";
    static constexpr auto Version = 1;
    static constexpr auto InstanceName = "Jacks View";
};

template <>
struct RenderableTrait<JacksView>
{
    static void RenderProperties(Handle, JacksView& view)
    {
        ImGui::InputText("Message", &view.Message);
        ImGui::SliderInt("Type", &view.Type, 0, 2);
    }

    static void Render(Handle, JacksView& view)
    {
        ImGui::Text("%s", view.Message.c_str());

        switch (view.Type)
        {
            case 0:
            {
                ZoneScopedN("Render Type A (PLOT)");
                if (ImPlot::BeginPlot("Sine Wave", ImVec2(-1, -1)))
                {
                    static float phase = 0.0f;
                    phase += 0.1f;
                    view.DataX.fill(0.0f);
                    view.DataY.fill(0.0f);
                    for (int i = 0; i < 100; i++)
                    {
                        view.DataX[i] = i * 0.1f;
                        view.DataY[i] = sinf(view.DataX[i] + phase);
                    }
                    ImPlot::PlotLine("Sine", view.DataX.data(), view.DataY.data(), 100);
                    ImPlot::EndPlot();
                }
                break;
            }
            case 1:
            {
                ZoneScopedN("Render Type B (BUTTON)");
                static bool show_imgui_demo = false;
                ImGui::Checkbox("Show ImGui Demo Window", &show_imgui_demo);
                if (show_imgui_demo)
                {
                    ImGui::ShowMetricsWindow(&show_imgui_demo);
                }
                break;
            }
            case 2:
            {
                ZoneScopedN("Render Type C (SLIDER)");
                static float value = 0.0f;
                ImGui::SliderFloat("Adjust Value", &value, 0.0f, 100.0f);
                ImGui::Text("Current Value: %.2f", value);
                ImGui::ShowUserGuide();
                break;
            }
        }
    }
};

struct WelcomeScreen
{
    std::string Greeting = "Welcome to the ECS Probe!";
};

template <>
struct Archetype<WelcomeScreen>
{
    static constexpr auto Identifier = "HelloImGui.ecs_probe.WelcomeScreen";
    static constexpr auto Version = 1;
    static constexpr auto InstanceName = "Welcome Screen";
};

struct Workspace
{
    std::string Name;
    Registry registry;

    Workspace(const std::string& name);
    void AddDefaultViews();
};

Registry g_registry;
std::vector<Workspace> g_workspaces;

template <>
struct RenderableTrait<WelcomeScreen>
{
    static void Render(Handle, WelcomeScreen& screen)
    {
        ImGui::Text("%s", screen.Greeting.c_str());

        if (ImGui::Button("Add New Workspace"))
        {
            g_workspaces.emplace_back("Workspace " + std::to_string(g_workspaces.size() + 1));
        }
    }
};

static Entity g_focused_entity = entt::null;
void SetFocusedEntity(Entity entity) { g_focused_entity = entity; }
Entity GetFocusedEntity() { return g_focused_entity; }

struct PropertyInspector
{
    int dummy;
};

template <>
struct Archetype<PropertyInspector>
{
    static constexpr auto Identifier = "HelloImGui.ecs_probe.PropertyInspector";
    static constexpr auto Version = 1;
    static constexpr auto InstanceName = "Property Inspector";
};

template <typename T>
Entity CreateView(Registry& registry, std::string dockspace_name = "NestedDockSpace")
{
    ZoneScopedN("CreateView");
    auto entity = registry.create();

    auto dockable_window = std::make_shared<HelloImGui::DockableWindow>();
    // generate a uuid for the window, using std::random_device and std::mt19937
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_int_distribution<int> dist(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 8; i++)
    {
        ss << std::hex << dist(mt);
    }
    std::string uuid = ss.str();
    dockable_window->label = std::format("{}###{}", Archetype<T>::InstanceName, uuid);
    dockable_window->dockSpaceName = dockspace_name;
    dockable_window->customTitleBarContextFunction =
        [entity, &registry](std::shared_ptr<HelloImGui::DockableWindow> window)
    {
        if (ImGui::MenuItem("Focus"))
        {
            SetFocusedEntity(entity);
        }
        if (ImGui::MenuItem("Close"))
        {
            window->isVisible = false;
        }
        if (ImGui::MenuItem("Reset Position"))
        {
            window->windowPosition = ImVec2(0, 0);
            window->windowSize = ImVec2(0, 0);
        }
        {
            static std::string shortLabel;

            const std::string& label = window->label;

            const bool is_mod = ImGui::IsKeyDown(ImGuiKey_ModShift);

            size_t separatorPos = label.find("###");
            std::string visibleLabel =
                (!is_mod && separatorPos != std::string::npos) ? label.substr(0, separatorPos) : label;

            if (shortLabel.empty() || shortLabel != visibleLabel)
            {
                shortLabel = visibleLabel;
            }

            static char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "%s", shortLabel.c_str());
            if (ImGui::InputText("##Rename",
                                 buffer,
                                 sizeof(buffer) / sizeof(buffer[0]),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string newLabel = std::string(buffer);
                if (separatorPos != std::string::npos)
                {
                    newLabel += label.substr(separatorPos);  // Preserve ID
                }
                window->label = newLabel;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                ImGui::CloseCurrentPopup();
            }
        }
    };

    registry.emplace<DockableWindowHandle>(entity, dockable_window);
    registry.emplace<T>(entity);
    return entity;
}

template <>
struct RenderableTrait<PropertyInspector>
{
    static void Render(Handle h, PropertyInspector& panel)
    {
        if (auto entity = GetFocusedEntity(); entity != entt::null)
        {
            for (const auto& [id, storage] : h.registry()->storage())
            {
                if (storage.contains(entity))
                {
                    auto type_id = storage.info();
                    ImGui::Separator();
                    ImGui::Text("Component: %s", std::string(type_id.name()).c_str());
                    const auto meta_type = entt::resolve(type_id);
                    if (meta_type)
                    {
                        auto render_properties_func = meta_type.func("RenderProperties"_hs);
                        if (render_properties_func)
                        {
                            render_properties_func.invoke({}, entt::handle(*h.registry(), entity));
                        }
                    }
                }
            }
        }
    }
};

template <typename T>
void DispatchRenderable(Registry& registry)
{
    for (auto [entity, dockable_window_handle, view] : registry.view<DockableWindowHandle, T>().each())
    {
        if (ImGui::Begin(dockable_window_handle.Window->label.c_str()))
        {
            RenderableTrait<T>::Render(entt::handle(registry, entity), view);
            if constexpr (requires { RenderableTrait<T>::RenderProperties; })
            {
                if (ImGui::IsWindowFocused())
                {
                    SetFocusedEntity(entity);
                }
            }
        }
        ImGui::End();
    }
}

Entity CreateDockspace(Registry& registry, std::string label, std::string dockspace_name)
{
    auto entity = registry.create();
    auto dockable_window = std::make_shared<HelloImGui::DockableWindow>();
    dockable_window->label = label;
    dockable_window->dockSpaceName = dockspace_name;
    dockable_window->dockingParams =
        HelloImGui::DockingParams{.dockingSplits = {
                                      {label, label + "_left", ImGuiDir_Left, 0.2f, 0, ImVec2(200, 100)},
                                      {label, label + "_right", ImGuiDir_Right, 0.4f, 0, ImVec2(200, 100)},
                                      {label, label + "_top", ImGuiDir_Up, 0.2f, 0, ImVec2(400, 100)},
                                      {label, label + "_bottom", ImGuiDir_Down, 0.2f, 0, ImVec2(100, 300)},
                                  }};
    dockable_window->customTitleBarContextFunction = [](std::shared_ptr<HelloImGui::DockableWindow> window)
    {
        if (ImGui::MenuItem("Reset Layout"))
        {
            window->dockingParams.layoutReset = true;
        }
    };
    registry.emplace<DockableWindowHandle>(entity, dockable_window);
    return entity;
}

template <typename T>
void RenderThunk(Handle handle)
{
    auto& component = handle.get<T>();
    RenderableTrait<T>::Render(handle, component);
}

template <typename T>
void RenderPropertiesThunk(Handle handle)
{
    auto& component = handle.get<T>();
    RenderableTrait<T>::RenderProperties(handle, component);
}

std::map<entt::id_type, void (*)(Registry&)> g_renderables;

template <typename T>
void RegisterRenderable()
{
    auto m = entt::meta_factory<T>{};
    if constexpr (requires { RenderableTrait<T>::Render; })
    {
        m.template func<&RenderThunk<T>>("Render"_hs);
        g_renderables[entt::type_hash<T>()] = &DispatchRenderable<T>;
    }
    if constexpr (requires { RenderableTrait<T>::RenderProperties; })
        m.template func<&RenderPropertiesThunk<T>>("RenderProperties"_hs);
}

void RenderDockableWindows(Registry& registry)
{
    ZoneScopedN("RenderDockableWindows");
    for (const auto& [type_id, render_func] : g_renderables)
    {
        render_func(registry);
    }
}

Workspace::Workspace(const std::string& name) : Name(name)
{
    // #############################################################################################
    //  Part 1: Define the application state, fill the status and menu bars, load additional font
    // #############################################################################################

    CreateDockspace(g_registry, std::format("NestedDockSpace_{}", Name), "MainDockSpace");
    AddDefaultViews();
}

void Workspace::AddDefaultViews()
{
    CreateView<JacksView>(this->registry, std::format("NestedDockSpace_{}", Name));
    CreateView<PropertyInspector>(this->registry, std::format("NestedDockSpace_{}_right", Name));
    CreateView<JacksView>(this->registry, std::format("NestedDockSpace_{}_left", Name));
    CreateView<JacksView>(this->registry, std::format("NestedDockSpace_{}_bottom", Name));
}

int main(int, char**)
{
    HelloImGui::CreateContext();

    RegisterRenderable<JacksView>();
    RegisterRenderable<PropertyInspector>();
    RegisterRenderable<WelcomeScreen>();
    CreateView<WelcomeScreen>(g_registry, "MainDockSpace");

    g_workspaces.emplace_back("Workspace 1");

    // Hello ImGui params (they hold the settings as well as the Gui callbacks)
    HelloImGui::RunnerParams runnerParams;
    runnerParams.appWindowParams.windowTitle = "Docking Demo";
    runnerParams.imGuiWindowParams.menuAppTitle = "Docking Demo";
    runnerParams.appWindowParams.windowGeometry.size = {1200, 1000};
    runnerParams.appWindowParams.restorePreviousGeometry = true;

    // Our application uses a borderless window, but is movable/resizable
    runnerParams.appWindowParams.borderless = false;

    //
    // Status bar
    //
    // We use the default status bar of Hello ImGui
    runnerParams.imGuiWindowParams.showStatusBar = true;

    //
    // Menu bar
    //
    // Here, we fully customize the menu bar:
    // by setting `showMenuBar` to true, and `showMenu_App` and `showMenu_View` to false,
    // HelloImGui will display an empty menu bar, which we can fill with our own menu items via the callback
    // `ShowMenus`
    runnerParams.imGuiWindowParams.showMenuBar = true;
    runnerParams.imGuiWindowParams.showMenu_App = true;
    runnerParams.imGuiWindowParams.showMenu_View = true;

    runnerParams.callbacks.PostRenderDockableWindows = [&]()
    {
        RenderDockableWindows(g_registry);
        for (auto& workspace : g_workspaces)
        {
            RenderDockableWindows(workspace.registry);
        }
    };
    runnerParams.callbacks.PostInit = []() { ImPlot::CreateContext(); };

    // ###############################################################################################
    //  Part 2: Define the application layout and windows
    // ###############################################################################################

    // First, tell HelloImGui that we want full screen dock space (this will create "MainDockSpace")
    runnerParams.imGuiWindowParams.defaultImGuiWindowType =
        HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    // In this demo, we also demonstrate multiple viewports: you can drag windows outside out the main window
    // in order to put their content into new native windows
    runnerParams.imGuiWindowParams.enableViewports = true;
    // Set the default layout
    runnerParams.dockingParams = HelloImGui::DockingParams{
        .dockingSplits =
            {
                {"MainDockSpace", "MainDockSpace_left", ImGuiDir_Left, 0.2f, 0, ImVec2(200, 100)},
                {"MainDockSpace", "MainDockSpace_right", ImGuiDir_Right, 0.4f, 0, ImVec2(200, 100)},
                {"MainDockSpace", "MainDockSpace_top", ImGuiDir_Up, 0.2f, 0, ImVec2(400, 100)},
                {"MainDockSpace", "MainDockSpace_bottom", ImGuiDir_Down, 0.2f, 0, ImVec2(100, 300)},
            },
        .mainDockSpaceNodeFlags = ImGuiDockNodeFlags_AutoHideTabBar,
    };

    // uncomment the next line if you want to always start with the layout defined in the code
    //     (otherwise, modifications to the layout applied by the user layout will be remembered)
    runnerParams.dockingParams.layoutCondition = HelloImGui::DockingLayoutCondition::ApplicationStart;
    runnerParams.iniFolderType = HelloImGui::IniFolderType::AppUserConfigFolder;

    // runnerParams.iniFilename: this will be the name of the ini file in which the settings
    // will be stored.
    // In this example, the subdirectory Docking_Demo will be created under the folder defined
    // by runnerParams.iniFolderType.
    //
    // Note: if iniFilename is left empty, the name of the ini file will be derived
    // from appWindowParams.windowTitle
    runnerParams.iniFilename = "Docking_Demo/Docking_demo.ini";

    // ###############################################################################################
    //  Part 4: Run the app
    // ###############################################################################################
    HelloImGui::DeleteIniSettings(runnerParams);

    // Optional: choose the backend combination
    // ----------------------------------------
    // runnerParams.platformBackendType = HelloImGui::PlatformBackendType::Sdl;
    // runnerParams.rendererBackendType = HelloImGui::RendererBackendType::Vulkan;

    HelloImGui::Run(runnerParams);  // Note: with ImGuiBundle, it is also possible to use ImmApp::Run(...)

    return 0;
}
