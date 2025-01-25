

#include "hello_imgui/hello_imgui.h"
#include "hello_imgui/icons_vscodicons.h"

#include "nlohmann/json.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"

#include "GLFW/glfw3.h"

#include "provider.hpp"
#include "lib.hpp"

#include "view.hpp"
#include "provider_selector.hpp"

int Provider::ctr = 0;



std::unordered_map<std::string, int> NamedInstance::m_instanceCount;

class FloatProvider : public Provider {
public:
  FloatProvider() : Provider("FloatProvider", "MainDockSpace") {}
};

class App {
public:
  App() : m_providerSelectorView("MainDockSpace") {
    workspace_lib::RegisterProviderType<FloatProvider>("FloatProvider");
  }

  void ShowMenuGui()
  {
    if (ImGui::BeginMenu("Layout"))
    {
      if (ImGui::MenuItem("Some button"))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Some button");
      ImGui::EndMenu();
    }
  }

  void ShowAppMenuItems()
  {
    if (ImGui::MenuItem("A Custom app menu item"))
      HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on A Custom app menu item");
  }

  void ShowTopToolbar() {
    if (ImGui::Button(ICON_VS_DEBUG_DISCONNECT))
      HelloImGui::GetRunnerParams()->appShallExit = true;
    ImGui::SameLine();
    if (ImGui::Button(ICON_VS_HOME))
      HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Home in the top toolbar");
    ImGui::SameLine();
    if (ImGui::Button(ICON_VS_SAVE))
      HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Save in the top toolbar");
    
    ImGui::SameLine();
    // green tint
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
    if (ImGui::Button(ICON_VS_ACCOUNT))
      HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Address Book in the top toolbar");
    ImGui::PopStyleColor(2);
  }

  ProviderSelectorView m_providerSelectorView; 
};

static void LoadFonts() {

  auto runnerParams = HelloImGui::GetRunnerParams();
  runnerParams->dpiAwareParams.onlyUseFontDpiResponsive = true;

  runnerParams->callbacks.defaultIconFont = HelloImGui::DefaultIconFont::VsCodeIcons;
  HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();
}

int main(int, char**)
{
    App app;

    // Hello ImGui params (they hold the settings as well as the Gui callbacks)
    HelloImGui::RunnerParams runnerParams;
    runnerParams.appWindowParams.windowTitle = "Workspace Demo";
    runnerParams.imGuiWindowParams.menuAppTitle = "Workspace Demo";
    runnerParams.appWindowParams.windowGeometry.size = {1200, 1000};
    runnerParams.appWindowParams.restorePreviousGeometry = true;

    // Our application uses a borderless window, but is movable/resizable
    runnerParams.appWindowParams.borderless = false;
    runnerParams.appWindowParams.borderlessMovable = true;
    runnerParams.appWindowParams.borderlessResizable = true;
    runnerParams.appWindowParams.borderlessClosable = true;

    //
    // Status bar
    //
    // We use the default status bar of Hello ImGui
    runnerParams.imGuiWindowParams.showStatusBar = true;
    // uncomment next line in order to hide the FPS in the status bar
    // runnerParams.imGuiWindowParams.showStatusFps = false;

    //
    // Menu bar
    //
    // Here, we fully customize the menu bar:
    // by setting `showMenuBar` to true, and `showMenu_App` and `showMenu_View` to false,
    // HelloImGui will display an empty menu bar, which we can fill with our own menu items via the callback `ShowMenus`
    runnerParams.imGuiWindowParams.showMenuBar = true;
    runnerParams.imGuiWindowParams.showMenu_App = true;
    runnerParams.imGuiWindowParams.showMenu_View = true;
    runnerParams.imGuiWindowParams.showMenu_View_Themes = true;
    // Inside `ShowMenus`, we can call `HelloImGui::ShowViewMenu` and `HelloImGui::ShowAppMenu` if desired
    runnerParams.callbacks.ShowMenus = [&app]() { app.ShowMenuGui(); };
    // Optional: add items to Hello ImGui default App menu
    runnerParams.callbacks.ShowAppMenuItems = [&app]() { app.ShowAppMenuItems(); };

    //
    // Top and bottom toolbars
    //
    // toolbar options
    HelloImGui::EdgeToolbarOptions edgeToolbarOptions;
    edgeToolbarOptions.sizeEm = 2.0f;
    edgeToolbarOptions.WindowBg = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
    // top toolbar
    runnerParams.callbacks.AddEdgeToolbar(
        HelloImGui::EdgeToolbarType::Top,
        [&app]() { app.ShowTopToolbar(); },
        edgeToolbarOptions
    );

    // remove any providers that were requested to be removed
    runnerParams.callbacks.LoadAdditionalFonts = [&]() { LoadFonts(); };

    runnerParams.callbacks.PreNewFrame = [&]() { 
      workspace_lib::RemoveProviders();
    };

    runnerParams.callbacks.PostInit = []() {
      auto glfw_window = HelloImGui::GetRunnerParams()->backendPointers.glfwWindow;
      if (glfw_window) {
        // log files dropped
        glfwSetDropCallback(static_cast<GLFWwindow*>(glfw_window), [](GLFWwindow* window, int count, const char** paths) {
          for (int i = 0; i < count; i++) {
            printf("Dropped file: %s\n", paths[i]);
          }
        });
      }
    };

    //
    // Load user settings at `PostInit` and save them at `BeforeExit`
    //
    // runnerParams.callbacks.PostInit = [&appState]   { LoadMyAppSettings(appState);};
    // runnerParams.callbacks.BeforeExit = [&appState] { SaveMyAppSettings(appState);};

    //###############################################################################################
    // Part 2: Define the application layout and windows
    //###############################################################################################

    // First, tell HelloImGui that we want full screen dock space (this will create "MainDockSpace")
    runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    // In this demo, we also demonstrate multiple viewports: you can drag windows outside out the main window in order to put their content into new native windows
    runnerParams.imGuiWindowParams.enableViewports = true;
    // Set the default layout
    // runnerParams.dockingParams = CreateDefaultLayout(appState);
    // Add alternative layouts
    //runnerParams.alternativeDockingLayouts = CreateAlternativeLayouts(appState);

    // uncomment the next line if you want to always start with the layout defined in the code
    //     (otherwise, modifications to the layout applied by the user layout will be remembered)
    runnerParams.dockingParams.layoutCondition = HelloImGui::DockingLayoutCondition::FirstUseEver;

    //###############################################################################################
    // Part 3: Where to save the app settings
    //###############################################################################################
    // By default, HelloImGui will save the settings in the current folder. This is convenient when developing,
    // but not so much when deploying the app.
    //     You can tell HelloImGui to save the settings in a specific folder: choose between
    //         CurrentFolder
    //         AppUserConfigFolder
    //         AppExecutableFolder
    //         HomeFolder
    //         TempFolder
    //         DocumentsFolder
    //
    //     Note: AppUserConfigFolder is:
    //         AppData under Windows (Example: C:\Users\[Username]\AppData\Roaming)
    //         ~/.config under Linux
            // "~/Library/Application Support" under macOS or iOS
    runnerParams.iniFolderType = HelloImGui::IniFolderType::AppUserConfigFolder;

    // runnerParams.iniFilename: this will be the name of the ini file in which the settings
    // will be stored.
    // In this example, the subdirectory Docking_Demo will be created under the folder defined
    // by runnerParams.iniFolderType.
    //
    // Note: if iniFilename is left empty, the name of the ini file will be derived
    // from appWindowParams.windowTitle
    runnerParams.iniFilename = "Workspace_Demo/Workspace_Demo.ini";

    //###############################################################################################
    // Part 4: Run the app
    //###############################################################################################
    // HelloImGui::DeleteIniSettings(runnerParams);

    // Optional: choose the backend combination
    // ----------------------------------------
    //runnerParams.platformBackendType = HelloImGui::PlatformBackendType::Sdl;
    //runnerParams.rendererBackendType = HelloImGui::RendererBackendType::Vulkan;

    HelloImGui::Run(runnerParams); // Note: with ImGuiBundle, it is also possible to use ImmApp::Run(...)

    return 0;
}