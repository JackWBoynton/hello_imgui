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

#include "hello_imgui/hello_imgui.h"
#include "hello_imgui/renderer_backend_options.h"
#include "hello_imgui/icons_font_awesome_6.h"
#include "nlohmann/json.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"

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


//////////////////////////////////////////////////////////////////////////
//    Our Application State
//////////////////////////////////////////////////////////////////////////
struct MyAppSettings
{
    HelloImGui::InputTextData motto = HelloImGui::InputTextData(
        "Hello, Dear ImGui\n"
        "Unleash your creativity!\n",
        true, // multiline
        ImVec2(14.f, 3.f) // initial size (in em)
    );
    int value = 10;
};


// the biggest assumption here is that ALL Labels are unique
template<typename Derived>
struct View : public HelloImGui::DockableWindow {
    void Render() { static_cast<Derived*>(this)->Render(); }

    View(const std::string& label, const std::string& dockSpaceName): HelloImGui::DockableWindow(label, dockSpaceName, "", [this] { Render(); }) {
        // HelloImGui::AddDockableWindow(this);
    }

    ~View() {
        HelloImGui::RemoveDockableWindow(label);
    }
};


class JacksView : public View<JacksView>
{
public:
    JacksView(int ctr, std::string DockspaceName) : View<JacksView>("Jack " + std::to_string(ctr), DockspaceName) {}

    void Render()
    {
        ImGui::Text("Jacks");
    }
};

template<typename Derived>
struct ViewDockspace : public HelloImGui::DockableWindow {

    void Render() { static_cast<Derived*>(this)->Render(); }

    ViewDockspace(const std::string& label, const std::string& dockSpaceName, HelloImGui::DockingParams docking_params) : HelloImGui::DockableWindow(label, dockSpaceName, "", [this] { Render(); }) {
        dockingParams = docking_params;
        // HelloImGui::AddDockableWindow(this);
    }

    ~ViewDockspace() {
        HelloImGui::RemoveDockableWindow(label);
    }
};

class Provider : public ViewDockspace<Provider>
{
public:
    static constexpr auto DockspaceName = "NestedDockSpace"; // parent dockspace to put ourselves in
    static int ctr;
    Provider() : ViewDockspace<Provider>("Provider", DockspaceName, {{{"Provider", "ProviderCommandSpace", ImGuiDir_Down, 0.5f}}}) { }

    void Render()
    {
        if (ImGui::Button("Add Jack"))
        {
            jacks.emplace_back(std::make_unique<JacksView>(ctr++, "Provider"));
        }

        int i = 0;
        for (auto& jack : jacks)
        {
            if (ImGui::Button(("Focus Jack" + std::to_string(i)).c_str()))
            {
                jack->focusWindowAtNextFrame = true;
            }
            i++;
        }
    }

    std::vector<std::unique_ptr<JacksView>> jacks;
};

int Provider::ctr = 0;

static std::shared_ptr<Provider> s_provider;

struct AppState
{
    float f = 0.0f;
    int counter = 0;

    float rocket_launch_time = 0.f;
    float rocket_progress = 0.0f;

    enum class RocketState {
        Init,
        Preparing,
        Launched
    };
    RocketState rocket_state = RocketState::Init;

    MyAppSettings myAppSettings; // This values will be stored in the application settings

    ImFont *TitleFont;
    ImFont *ColorFont;
    ImFont *EmojiFont;
    ImFont *LargeIconFont;
};


//////////////////////////////////////////////////////////////////////////
//    Additional fonts handling
//////////////////////////////////////////////////////////////////////////
void LoadFonts(AppState& appState) // This is called by runnerParams.callbacks.LoadAdditionalFonts
{
    auto runnerParams = HelloImGui::GetRunnerParams();

    runnerParams->callbacks.defaultIconFont = HelloImGui::DefaultIconFont::FontAwesome6;
    // First, load the default font (the default font should be loaded first)
    HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();
    // Then load the other fonts
    appState.TitleFont = HelloImGui::LoadFont("fonts/DroidSans.ttf", 18.f);

    HelloImGui::FontLoadingParams fontLoadingParamsEmoji;
    appState.EmojiFont = HelloImGui::LoadFont("fonts/NotoEmoji-Regular.ttf", 24.f, fontLoadingParamsEmoji);

    HelloImGui::FontLoadingParams fontLoadingParamsLargeIcon;
    appState.LargeIconFont = HelloImGui::LoadFont("fonts/fontawesome-webfont.ttf", 24.f, fontLoadingParamsLargeIcon);
#ifdef IMGUI_ENABLE_FREETYPE
    // Found at https://www.colorfonts.wtf/
    HelloImGui::FontLoadingParams fontLoadingParamsColor;
    fontLoadingParamsColor.loadColor = true;
    appState.ColorFont = HelloImGui::LoadFont("fonts/Playbox/Playbox-FREE.otf", 24.f, fontLoadingParamsColor);
#endif
}


//////////////////////////////////////////////////////////////////////////
//    Save additional settings in the ini file
//////////////////////////////////////////////////////////////////////////
// This demonstrates how to store additional info in the application settings
// Use this sparingly!
// This is provided as a convenience only, and it is not intended to store large quantities of text data.

// Warning, the save/load function below are quite simplistic!
std::string MyAppSettingsToString(const MyAppSettings& myAppSettings)
{
    using namespace nlohmann;
    json j;
    j["motto"] = HelloImGui::InputTextDataToString(myAppSettings.motto);
    j["value"] = myAppSettings.value;
    j["num_jacks"] = Provider::ctr;
    return j.dump();
}
MyAppSettings StringToMyAppSettings(const std::string& s)
{
    if (s.empty())
        return MyAppSettings();
    MyAppSettings myAppSettings;
    using namespace nlohmann;
    try {
        json j = json::parse(s);
        myAppSettings.motto = HelloImGui::InputTextDataFromString(j["motto"].get<std::string>());
        myAppSettings.value = j.value("value", 10); // default value is 10
        Provider::ctr = j.value("num_jacks", 0); // default value is 0
        if (!s_provider)
            s_provider = std::make_shared<Provider>();
        for (int i = 0; i < Provider::ctr; i++)
            s_provider->jacks.emplace_back(std::make_unique<JacksView>(i, "Provider"));
    }
    catch (json::exception& e)
    {
        HelloImGui::Log(HelloImGui::LogLevel::Error, "Error while parsing user settings: %s", e.what());
    }
    return myAppSettings;
}

// Note: LoadUserSettings() and SaveUserSettings() will be called in the callbacks `PostInit` and `BeforeExit`:
//     runnerParams.callbacks.PostInit = [&appState]   { LoadMyAppSettings(appState);};
//     runnerParams.callbacks.BeforeExit = [&appState] { SaveMyAppSettings(appState);};
void LoadMyAppSettings(AppState& appState) //
{
    appState.myAppSettings = StringToMyAppSettings(HelloImGui::LoadUserPref("MyAppSettings"));
}
void SaveMyAppSettings(const AppState& appState)
{
    HelloImGui::SaveUserPref("MyAppSettings", MyAppSettingsToString(appState.myAppSettings));
}

//////////////////////////////////////////////////////////////////////////
//    Gui functions used in this demo
//////////////////////////////////////////////////////////////////////////

void ShowTitle(AppState& appState, const char* title)
{
    ImGui::PushFont(appState.TitleFont);
    // ImGui::PushFontSize(appState.TitleFont->DefaultSize);  // incompatible with vcpkg's version
    ImGui::Text("%s", title);
    // ImGui::PopFontSize();  // incompatible with vcpkg's version
    ImGui::PopFont();
}

// Display a button that will hide the application window
void DemoHideWindow(AppState& appState)
{
    ShowTitle(appState, "Hide app window");
    static double lastHideTime = -1.;
    if (ImGui::Button("Hide"))
    {
        lastHideTime =  ImGui::GetTime();
        HelloImGui::GetRunnerParams()->appWindowParams.hidden = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("By clicking this button, you can hide the window for 3 seconds.");
    if (lastHideTime > 0.)
    {
        double now = ImGui::GetTime();
        if (now - lastHideTime > 3.)
        {
            lastHideTime = -1.;
            HelloImGui::GetRunnerParams()->appWindowParams.hidden = false;
        }
    }
}

// Display a button that will add another dockable window during execution
void DemoShowAdditionalWindow(AppState& appState)
{
    // In order to add a dockable window during execution, you should use
    //     HelloImGui::AddDockableWindow()
    // Note: you should not modify manually the content of runnerParams.dockingParams.dockableWindows
    //       (since HelloImGui is constantly looping on it)
    ShowTitle(appState, "Dynamically add window");

    const char* windowName = "Additional Window";
    if (ImGui::Button("Show additional window"))
    {
        static std::shared_ptr<HelloImGui::DockableWindow> s_additionalWindow;
        if (!s_additionalWindow)
        {
            s_additionalWindow = std::make_shared<HelloImGui::DockableWindow>();
            s_additionalWindow->label = windowName;
            s_additionalWindow->includeInViewMenu = false; // do not include in the View menu
            s_additionalWindow->rememberIsVisible = false; // do not remember if the window is visible
            s_additionalWindow->dockSpaceName = "MiscSpace"; // dock space to put the window in
            s_additionalWindow->GuiFunction = [] {
                ImGui::Text("This is the additional window");
            };
        }

        HelloImGui::AddDockableWindow(
            s_additionalWindow,
            false  // forceDockspace=false: means that the window will be docked to the last space it was docked to
            // i.e. dockSpaceName is ignored if the user previously moved the window to another space
        );
    }
    ImGui::SetItemTooltip("By clicking this button, you can show an additional window");

    if (ImGui::Button("Remove additional window"))
        HelloImGui::RemoveDockableWindow(windowName);
    ImGui::SetItemTooltip("By clicking this button, you can remove the additional window");
}

void DemoLogs(AppState& appState)
{
    ShowTitle(appState, "Log Demo");

    ImGui::BeginGroup();
    // Edit a float using a slider from 0.0f to 1.0f
    bool changed = ImGui::SliderFloat("float", &appState.f, 0.0f, 1.0f);
    if (changed)
        HelloImGui::Log(HelloImGui::LogLevel::Warning, "state.f was changed to %f", appState.f);

    // Buttons return true when clicked (most widgets return true when edited/activated)
    if (ImGui::Button("Button"))
    {
        appState.counter++;
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Button was pressed");
    }

    ImGui::SameLine();
    ImGui::Text("counter = %d", appState.counter);
    ImGui::EndGroup();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("These widgets will interact with the log window");
}

void DemoUserSettings(AppState& appState)
{
    ShowTitle(appState, "User settings");
    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(HelloImGui::EmSize(7.f));


    ImGui::SliderInt("Value", &appState.myAppSettings.value, 0, 100);
    HelloImGui::InputTextResizable("Motto", &appState.myAppSettings.motto);
    ImGui::Text("(this text widget is resizable)");
    ImGui::EndGroup();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The values below are stored in the application settings ini file and restored at startup");
}

void DemoRocket(AppState& appState)
{
    ShowTitle(appState, "Status Bar Demo");
    ImGui::BeginGroup();
    if (appState.rocket_state == AppState::RocketState::Init)
    {
        if (ImGui::Button(ICON_FA_ROCKET" Launch rocket"))
        {
            appState.rocket_launch_time = (float)ImGui::GetTime();
            appState.rocket_state = AppState::RocketState::Preparing;
            HelloImGui::Log(HelloImGui::LogLevel::Warning, "Rocket is being prepared");
        }
    }
    else if (appState.rocket_state == AppState::RocketState::Preparing)
    {
        ImGui::Text("Please Wait");
        appState.rocket_progress = (float)(ImGui::GetTime() - appState.rocket_launch_time) / 3.f;
        if (appState.rocket_progress >= 1.0f)
        {
            appState.rocket_state = AppState::RocketState::Launched;
            HelloImGui::Log(HelloImGui::LogLevel::Warning, "Rocket was launched");
        }
    }
    else if (appState.rocket_state == AppState::RocketState::Launched)
    {
        ImGui::Text(ICON_FA_ROCKET " Rocket launched");
        if (ImGui::Button("Reset Rocket"))
        {
            appState.rocket_state = AppState::RocketState::Init;
            appState.rocket_progress = 0.f;
        }
    }
    ImGui::EndGroup();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Look at the status bar after clicking");
}

void DemoDockingFlags(AppState& appState)
{
    ShowTitle(appState, "Main dock space node flags");
    ImGui::TextWrapped(R"(
This will edit the ImGuiDockNodeFlags for "MainDockSpace".
Most flags are inherited by children dock spaces.
    )");

    struct DockFlagWithInfo {
        ImGuiDockNodeFlags flag;
        std::string label;
        std::string tip;
    };
    std::vector<DockFlagWithInfo> all_flags = {
        {ImGuiDockNodeFlags_NoSplit, "NoSplit", "prevent Dock Nodes from being split"},
        {ImGuiDockNodeFlags_NoResize, "NoResize", "prevent Dock Nodes from being resized"},
        {ImGuiDockNodeFlags_AutoHideTabBar, "AutoHideTabBar",
         "show tab bar only if multiple windows\n"
         "You will need to restore the layout after changing (Menu \"View/Restore Layout\")"},
        {ImGuiDockNodeFlags_NoDockingInCentralNode, "NoDockingInCentralNode",
         "prevent docking in central node\n"
         "(only works with the main dock space)"},
        // {ImGuiDockNodeFlags_PassthruCentralNode, "PassthruCentralNode", "advanced"},
        // {ImGuiDockNodeFlags_KeepAliveOnly, "KeepAliveOnly", "warning, this will prevent sub dock nodes from rendering"},
    };

    auto & mainDockSpaceNodeFlags = HelloImGui::GetRunnerParams()->dockingParams.mainDockSpaceNodeFlags;
    for (const auto& flag: all_flags)
    {
        ImGui::CheckboxFlags(flag.label.c_str(), &mainDockSpaceNodeFlags, flag.flag);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", flag.tip.c_str());
    }
}

void GuiWindowLayoutCustomization(AppState& appState)
{
    ShowTitle(appState, "Switch between layouts");
    ImGui::Text("with the menu \"View/Layouts\"");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Each layout remembers separately the modifications applied by the user, \nand the selected layout is restored at startup");
    ImGui::Separator();

    ImGui::PushFont(appState.TitleFont); ImGui::Text("Change the theme"); ImGui::PopFont();
    ImGui::Text("with the menu \"View/Theme\"");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The selected theme is remembered and restored at startup");
    ImGui::Separator();

    DemoDockingFlags(appState);
    ImGui::Separator();
}


void GuiWindowAlternativeTheme(AppState& appState)
{
    // Since this window applies a theme, We need to call "ImGui::Begin" ourselves so
    // that we can apply the theme before opening the window.
    //
    // In order to obtain this, we applied the following option to the window
    // that displays this Gui:
    //     alternativeThemeWindow.callBeginEnd = false;

    // Apply the theme before opening the window
    ImGuiTheme::ImGuiTweakedTheme tweakedTheme;
    tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_WhiteIsWhite;
    tweakedTheme.Tweaks.Rounding = 0.0f;
    ImGuiTheme::PushTweakedTheme(tweakedTheme);

    // Open the window
    bool windowOpened = ImGui::Begin("Alternative Theme");
    if (windowOpened)
    {
        // Display some widgets
        ImGui::PushFont(appState.TitleFont); ImGui::Text("Alternative Theme"); ImGui::PopFont();
        ImGui::Text("This window uses a different theme");
        ImGui::SetItemTooltip("    ImGuiTheme::ImGuiTweakedTheme tweakedTheme;\n"
                              "    tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_WhiteIsWhite;\n"
                              "    tweakedTheme.Tweaks.Rounding = 0.0f;\n"
                              "    ImGuiTheme::PushTweakedTheme(tweakedTheme);");

        if (ImGui::CollapsingHeader("Basic Widgets", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static bool checked = true;
            ImGui::Checkbox("Checkbox", &checked);

            if (ImGui::Button("Button"))
                HelloImGui::Log(HelloImGui::LogLevel::Info, "Button was pressed");
            ImGui::SetItemTooltip("This is a button");

            static int radio = 0;
            ImGui::RadioButton("Radio 1", &radio, 0); ImGui::SameLine();
            ImGui::RadioButton("Radio 2", &radio, 1); ImGui::SameLine();
            ImGui::RadioButton("Radio 3", &radio, 2);

            // Haiku
            {
                // Display a image of the haiku below with Japanese characters
                // with an informative tooltip
                float haikuImageHeight = HelloImGui::EmSize(5.f);
                HelloImGui::ImageFromAsset("images/haiku.png", ImVec2(0.f, haikuImageHeight));
                ImGui::SetItemTooltip(R"(
Extract from Wikipedia
-------------------------------------------------------------------------------

In early 1686, Bashō composed one of his best-remembered haiku:

        furu ike ya / kawazu tobikomu / mizu no oto

   an ancient pond / a frog jumps in / the splash of water

This poem became instantly famous.

-------------------------------------------------------------------------------

This haiku is here rendered as an image, mainly to preserve space,
because adding a Japanese font to the project would enlarge its size.
Handling Japanese font is of course possible within ImGui / Hello ImGui!
            )");

                // Display the haiku text as an InputTextMultiline
                static std::string poem =
                    "   Old Pond\n"
                    "  Frog Leaps In\n"
                    " Water's Sound\n"
                    "\n"
                    "      Matsuo Bashō - 1686";
                ImGui::InputTextMultiline("##Poem", &poem, HelloImGui::EmToVec2(15.f, 5.5f));
            }

            // A popup with a modal window
            if (ImGui::Button("Open Modal"))
                ImGui::OpenPopup("MyModal");
            if (ImGui::BeginPopupModal("MyModal", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("This is a modal window");
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            static std::string text = "Hello, world!";
            ImGui::InputText("Input text", &text);

            if (ImGui::TreeNode("Text Display"))
            {
                ImGui::Text("Hello, world!");
                ImGui::TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), "Some text");
                ImGui::TextDisabled("Disabled text");
                ImGui::TextWrapped("This is a long text that will be wrapped in the window");
                ImGui::TreePop();
            }
        }
    }
    // Close the window
    ImGui::End();

    // Restore the theme
    ImGuiTheme::PopTweakedTheme();
}

void DemoAssets(AppState& appState)
{
    ShowTitle(appState, "Image From Asset");

    HelloImGui::BeginGroupColumn();
    ImGui::Dummy(HelloImGui::EmToVec2(0.f, 0.45f));
    ImGui::Text("Hello");
    HelloImGui::EndGroupColumn();
    HelloImGui::ImageFromAsset("images/world.png", HelloImGui::EmToVec2(2.5f, 2.5f));
}

void DemoFonts(AppState& appState)
{
    ShowTitle(appState, "Fonts");
    ImGui::TextWrapped("Mix icons " ICON_FA_FACE_SMILE " and text " ICON_FA_ROCKET "");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Example with Font Awesome Icons");

    ImGui::Text("Emojis");

    ImGui::BeginGroup();
    {
        ImGui::PushFont(appState.EmojiFont);
        // ✌️ (Victory Hand Emoji)
        ImGui::Text(U8_TO_CHAR(u8"\U0000270C\U0000FE0F"));
        ImGui::SameLine();

        // ❤️ (Red Heart Emoji)
        ImGui::Text(U8_TO_CHAR(u8"\U00002764\U0000FE0F"));
        ImGui::SameLine();

#ifdef IMGUI_USE_WCHAR32
        // 🌴 (Palm Tree Emoji)
        ImGui::Text(U8_TO_CHAR(u8"\U0001F334"));
        ImGui::SameLine();

        // 🚀 (Rocket Emoji)
        ImGui::Text(U8_TO_CHAR(u8"\U0001F680"));
        ImGui::SameLine();
#endif

        ImGui::PopFont();
    }
    ImGui::EndGroup();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Example with NotoEmoji font");

#ifdef IMGUI_ENABLE_FREETYPE
    ImGui::Text("Colored Fonts");
    ImGui::PushFont(appState.ColorFont);
    ImGui::Text("COLOR!");
    ImGui::PopFont();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Example with Playbox-FREE.otf font");
#endif
}

void DemoThemes(AppState& appState)
{
    ShowTitle(appState, "Themes");
    auto& tweakedTheme = HelloImGui::GetRunnerParams()->imGuiWindowParams.tweakedTheme;

    ImGui::BeginGroup();
    ImVec2 buttonSize = HelloImGui::EmToVec2(7.f, 0.f);
    if (ImGui::Button("Cherry", buttonSize))
    {
        tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_Cherry;
        ImGuiTheme::ApplyTweakedTheme(tweakedTheme);
    }
    if (ImGui::Button("DarculaDarker", buttonSize))
    {
        tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_DarculaDarker;
        ImGuiTheme::ApplyTweakedTheme(tweakedTheme);
    }
    ImGui::EndGroup();
    if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "There are lots of other themes: look at the menu View/Theme\n"
                "The selected theme is remembered and restored at startup"
            );
}

// The Gui of the demo feature window
void GuiWindowDemoFeatures(AppState& appState)
{
    DemoFonts(appState);
    ImGui::Separator();
    DemoAssets(appState);
    ImGui::Separator();
    DemoLogs(appState);
    ImGui::Separator();
    DemoRocket(appState);
    ImGui::Separator();
    DemoUserSettings(appState);
    ImGui::Separator();
    DemoHideWindow(appState);
    ImGui::Separator();
    DemoShowAdditionalWindow(appState);
    ImGui::Separator();
    DemoThemes(appState);
    ImGui::Separator();
}

// The Gui of the status bar
void StatusBarGui(AppState& app_state)
{
    ImGui::Text("Using backend: %s", HelloImGui::GetBackendDescription().c_str());
    ImGui::SameLine();
    if (app_state.rocket_state == AppState::RocketState::Preparing)
    {
        ImGui::Text("  -  Rocket completion: ");
        ImGui::SameLine();
        ImGui::ProgressBar(app_state.rocket_progress, HelloImGui::EmToVec2(7.0f, 1.0f));
    }
}


// The menu gui
void ShowMenuGui(HelloImGui::RunnerParams& runnerParams)
{
    HelloImGui::ShowAppMenu(runnerParams);
    HelloImGui::ShowViewMenu(runnerParams);

    if (ImGui::BeginMenu("My Menu"))
    {
        bool clicked = ImGui::MenuItem("Test me", "", false);
        if (clicked)
        {
            HelloImGui::Log(HelloImGui::LogLevel::Warning, "It works");
        }
        ImGui::EndMenu();
    }
}

void ShowAppMenuItems()
{
    if (ImGui::MenuItem("A Custom app menu item"))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on A Custom app menu item");
}

void ShowTopToolbar(AppState& appState)
{
    ImGui::PushFont(appState.LargeIconFont);
    if (ImGui::Button(ICON_FA_POWER_OFF))
        HelloImGui::GetRunnerParams()->appShallExit = true;

    ImGui::SameLine(ImGui::GetWindowWidth() - HelloImGui::EmSize(7.f));
    if (ImGui::Button(ICON_FA_HOUSE))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Home in the top toolbar");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FLOPPY_DISK))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Save in the top toolbar");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ADDRESS_BOOK))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Address Book in the top toolbar");

    ImGui::SameLine(ImGui::GetWindowWidth() - HelloImGui::EmSize(2.f));
    ImGui::Text(ICON_FA_BATTERY_THREE_QUARTERS);
    ImGui::PopFont();
}

void ShowRightToolbar(AppState& appState)
{
    ImGui::PushFont(appState.LargeIconFont);
    if (ImGui::Button(ICON_FA_CIRCLE_ARROW_LEFT))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Circle left in the right toolbar");

    if (ImGui::Button(ICON_FA_CIRCLE_ARROW_RIGHT))
        HelloImGui::Log(HelloImGui::LogLevel::Info, "Clicked on Circle right in the right toolbar");
    ImGui::PopFont();
}

//////////////////////////////////////////////////////////////////////////
//    Docking Layouts and Docking windows
//////////////////////////////////////////////////////////////////////////

//
// 1. Define the Docking splits (two versions are available)
//
std::vector<HelloImGui::DockingSplit> CreateDefaultDockingSplits()
{
    //    Define the default docking splits,
    //    i.e. the way the screen space is split in different target zones for the dockable windows
    //     We want to split "MainDockSpace" (which is provided automatically) into three zones, like this:
    //
    //    ___________________________________________
    //    |        |                                |
    //    | Command|                                |
    //    | Space  |    MainDockSpace               |
    //    |------- |                                |
    //    |        |--------------------------------|
    //    |        |       CommandSpace2            |
    //    -------------------------------------------
    //    |     MiscSpace                           |
    //    -------------------------------------------
    //

    // Then, add a space named "MiscSpace" whose height is 25% of the app height.
    // This will split the preexisting default dockspace "MainDockSpace" in two parts.
    HelloImGui::DockingSplit splitMainMisc;
    splitMainMisc.initialDock = "MainDockSpace";
    splitMainMisc.newDock = "MiscSpace";
    splitMainMisc.direction = ImGuiDir_Down;
    splitMainMisc.ratio = 0.25f;

    // Then, add a space to the left which occupies a column whose width is 25% of the app width
    HelloImGui::DockingSplit splitMainCommand;
    splitMainCommand.initialDock = "MainDockSpace";
    splitMainCommand.newDock = "CommandSpace";
    splitMainCommand.direction = ImGuiDir_Left;
    splitMainCommand.ratio = 0.25f;

    // Then, add CommandSpace2 below MainDockSpace
    HelloImGui::DockingSplit splitMainCommand2;
    splitMainCommand2.initialDock = "MainDockSpace";
    splitMainCommand2.newDock = "CommandSpace2";
    splitMainCommand2.direction = ImGuiDir_Down;
    splitMainCommand2.ratio = 0.5f;

    std::vector<HelloImGui::DockingSplit> splits {splitMainMisc, splitMainCommand, splitMainCommand2};
    return splits;
}

std::vector<HelloImGui::DockingSplit> CreateAlternativeDockingSplits()
{
    //    Define alternative docking splits for the "Alternative Layout"
    //    ___________________________________________
    //    |                |                        |
    //    | Misc           |                        |
    //    | Space          |    MainDockSpace       |
    //    |                |                        |
    //    -------------------------------------------
    //    |                       |                 |
    //    |                       | Command         |
    //    |     CommandSpace      | Space2          |
    //    |                       |                 |
    //    -------------------------------------------

    HelloImGui::DockingSplit splitMainCommand;
    splitMainCommand.initialDock = "MainDockSpace";
    splitMainCommand.newDock = "CommandSpace";
    splitMainCommand.direction = ImGuiDir_Down;
    splitMainCommand.ratio = 0.5f;

    HelloImGui::DockingSplit splitMainCommand2;
    splitMainCommand2.initialDock = "CommandSpace";
    splitMainCommand2.newDock = "CommandSpace2";
    splitMainCommand2.direction = ImGuiDir_Right;
    splitMainCommand2.ratio = 0.4f;

    HelloImGui::DockingSplit splitMainMisc;
    splitMainMisc.initialDock = "MainDockSpace";
    splitMainMisc.newDock = "MiscSpace";
    splitMainMisc.direction = ImGuiDir_Left;
    splitMainMisc.ratio = 0.5f;

    std::vector<HelloImGui::DockingSplit> splits {splitMainCommand, splitMainCommand2, splitMainMisc};
    return splits;
}

std::vector<HelloImGui::DockingSplit> CreateNestedDockingSplits() {
    HelloImGui::DockingSplit splitMainTop;
    splitMainTop.initialDock = "NestedDockSpace";
    splitMainTop.newDock = "NestedDockSpaceTop";
    splitMainTop.direction = ImGuiDir_Up;
    splitMainTop.ratio = 0.5f;

    return {splitMainTop};
}

static std::shared_ptr<HelloImGui::DockableWindow> s_nestedWindow1;
static std::shared_ptr<HelloImGui::DockableWindow> s_nestedWindow2;

std::vector<std::shared_ptr<HelloImGui::DockableWindow>> CreateNestedDockableWindows(AppState& appState) {
    static bool first = true;
    if (first) {
        first = false;

        s_nestedWindow1 = std::make_shared<HelloImGui::DockableWindow>();
        s_nestedWindow1->label = "Nested Window 1";
        s_nestedWindow1->dockSpaceName = "NestedDockSpaceTop";
        s_nestedWindow1->GuiFunction = [] { ImGui::Text("This is the first nested window"); };
        
        s_nestedWindow2 = std::make_shared<HelloImGui::DockableWindow>();
        s_nestedWindow2->label = "Nested Window 2";
        s_nestedWindow2->dockSpaceName = "NestedDockSpace";
        s_nestedWindow2->GuiFunction = [&] { 
            ImGui::Text("This is the second nested window"); 
            if (!s_provider && ImGui::Button("Add Provider"))
                s_provider = std::make_shared<Provider>();
        };
    }

    return {s_nestedWindow1, s_nestedWindow2};
}

//
// 2. Define the Dockable windows
//
static std::shared_ptr<HelloImGui::DockableWindow> s_featuresDemoWindow;
static std::shared_ptr<HelloImGui::DockableWindow> s_layoutCustomizationWindow;
static std::shared_ptr<HelloImGui::DockableWindow> s_logsWindow;
static std::shared_ptr<HelloImGui::DockableWindow> s_dearImGuiDemoWindow;
static std::shared_ptr<HelloImGui::DockableWindow> s_alternativeThemeWindow;
static std::shared_ptr<HelloImGui::DockableWindow> s_nestedDockspaceWindow;


std::vector<std::shared_ptr<HelloImGui::DockableWindow>> CreateDockableWindows(AppState& appState)
{
    static bool first = true;
    if (first)
    {
        first = false;
        // A window named "FeaturesDemo" will be placed in "CommandSpace". Its Gui is provided by "GuiWindowDemoFeatures"
        s_featuresDemoWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_featuresDemoWindow->label = "Features Demo";
        s_featuresDemoWindow->dockSpaceName = "CommandSpace";
        s_featuresDemoWindow->GuiFunction = [&] { GuiWindowDemoFeatures(appState); };

        // A layout customization window will be placed in "MainDockSpace". Its Gui is provided by "GuiWindowLayoutCustomization"
        s_layoutCustomizationWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_layoutCustomizationWindow->label = "Layout customization";
        s_layoutCustomizationWindow->dockSpaceName = "MainDockSpace";
        s_layoutCustomizationWindow->GuiFunction = [&appState]() { GuiWindowLayoutCustomization(appState); };

        // A Log window named "Logs" will be placed in "MiscSpace". It uses the HelloImGui logger gui
        s_logsWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_logsWindow->label = "Logs";
        s_logsWindow->dockSpaceName = "MiscSpace";
        s_logsWindow->GuiFunction = [] { HelloImGui::LogGui(); };

        // A Window named "Dear ImGui Demo" will be placed in "MainDockSpace"
        s_dearImGuiDemoWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_dearImGuiDemoWindow->label = "Dear ImGui Demo";
        s_dearImGuiDemoWindow->dockSpaceName = "MainDockSpace";
        s_dearImGuiDemoWindow->imGuiWindowFlags = ImGuiWindowFlags_MenuBar;
        s_dearImGuiDemoWindow->GuiFunction = [] { ImGui::ShowDemoWindow(); };

        // alternativeThemeWindow
        // Since this window applies a theme, We need to call "ImGui::Begin" ourselves so
        // that we can apply the theme before opening the window.
        s_alternativeThemeWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_alternativeThemeWindow->callBeginEnd = false;
        s_alternativeThemeWindow->label = "Alternative Theme";
        s_alternativeThemeWindow->dockSpaceName = "CommandSpace2";
        s_alternativeThemeWindow->GuiFunction = [&appState]() { GuiWindowAlternativeTheme(appState); };

        // Add another window that acts as a dockspace
        // This window will be used to demonstrate nested dockspaces
        s_nestedDockspaceWindow = std::make_shared<HelloImGui::DockableWindow>();
        s_nestedDockspaceWindow->label = "NestedDockSpace";
        s_nestedDockspaceWindow->dockSpaceName = "MainDockSpace";
        s_nestedDockspaceWindow->GuiFunction = [] { 
            ImGui::Text("This is the nested dockspace window");
            static bool hasDockableWindow = false;
            if (!hasDockableWindow && ImGui::Button("Add dockable window")) {
                hasDockableWindow = true;
                static std::shared_ptr<HelloImGui::DockableWindow> s_newDockableWindow;
                if (!s_newDockableWindow)
                    s_newDockableWindow = std::make_shared<HelloImGui::DockableWindow>();
                s_newDockableWindow->label = "New Dockable Window";
                s_newDockableWindow->dockSpaceName = "NestedDockSpace asdf";
                s_newDockableWindow->GuiFunction = [] { ImGui::Text("This is a new dockable window"); };
                HelloImGui::AddDockableWindow(s_newDockableWindow);
            }

            if (!hasDockableWindow && ImGui::Button("Add window (invalid dockspace)")) {
                hasDockableWindow = true;
                static std::shared_ptr<HelloImGui::DockableWindow> s_newDockableWindow;
                if (!s_newDockableWindow)
                    s_newDockableWindow = std::make_shared<HelloImGui::DockableWindow>();
                s_newDockableWindow->label = "New Dockable Window";
                s_newDockableWindow->dockSpaceName = "asdf";
                s_newDockableWindow->GuiFunction = [] { ImGui::Text("This is a new (invalid dockspace) dockable window"); };
                HelloImGui::AddDockableWindow(s_newDockableWindow);
            }

            if (hasDockableWindow && ImGui::Button("Remove dockable window")) {
                HelloImGui::RemoveDockableWindow("New Dockable Window");
                hasDockableWindow = false;
            }
        };
        s_nestedDockspaceWindow->dockingParams.dockingSplits = CreateNestedDockingSplits();
        s_nestedDockspaceWindow->dockingParams.dockableWindows = CreateNestedDockableWindows(appState);
        s_nestedDockspaceWindow->dockingParams.mainDockSpaceNodeFlags = ImGuiDockNodeFlags_None;   
    }

    std::vector<std::shared_ptr<HelloImGui::DockableWindow>> dockableWindows {
        s_featuresDemoWindow,
        s_layoutCustomizationWindow,
        s_logsWindow,
        s_dearImGuiDemoWindow,
        s_alternativeThemeWindow,
        s_nestedDockspaceWindow
    };
    return dockableWindows;
}

//
// 3. Define the layouts:
//        A layout is stored inside DockingParams, and stores the splits + the dockable windows.
//        Here, we provide the default layout, and two alternative layouts.
//
HelloImGui::DockingParams CreateDefaultLayout(AppState& appState)
{
    HelloImGui::DockingParams dockingParams;
    // dockingParams.layoutName = "Default"; // By default, the layout name is already "Default"
    dockingParams.dockingSplits = CreateDefaultDockingSplits();
    dockingParams.dockableWindows = CreateDockableWindows(appState);
    return dockingParams;
}

std::vector<HelloImGui::DockingParams> CreateAlternativeLayouts(AppState& appState)
{
    HelloImGui::DockingParams alternativeLayout;
    {
        alternativeLayout.layoutName = "Alternative Layout";
        alternativeLayout.dockingSplits = CreateAlternativeDockingSplits();
        alternativeLayout.dockableWindows = CreateDockableWindows(appState);
    }
    HelloImGui::DockingParams tabsLayout;
    {
        tabsLayout.layoutName = "Tabs Layout";
        tabsLayout.dockableWindows = CreateDockableWindows(appState);
        // Force all windows to be presented in the MainDockSpace
        for (auto& window: tabsLayout.dockableWindows)
            window->dockSpaceName = "MainDockSpace";
        // In "Tabs Layout", no split is created
        tabsLayout.dockingSplits = {};
    }
    return {alternativeLayout, tabsLayout};
}


//////////////////////////////////////////////////////////////////////////
// Define the app initial theme
//////////////////////////////////////////////////////////////////////////
void SetupMyTheme()
{
    // Example of theme customization at App startup
    // This function is called in the callback `SetupImGuiStyle` in order to apply a custom theme:
    //     runnerParams.callbacks.SetupImGuiStyle = SetupMyTheme;

    // Apply default style
    HelloImGui::ImGuiDefaultSettings::SetupDefaultImGuiStyle();
    // Create a tweaked theme
    ImGuiTheme::ImGuiTweakedTheme tweakedTheme;
    tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_MaterialFlat;
    tweakedTheme.Tweaks.Rounding = 10.0f;
    // Apply the tweaked theme
    ImGuiTheme::ApplyTweakedTheme(tweakedTheme); // Note: you can also push/pop the theme in order to apply it only to a specific part of the Gui: ImGuiTheme::PushTweakedTheme(tweakedTheme) / ImGuiTheme::PopTweakedTheme()
    // Then apply further modifications to ImGui style
    ImGui::GetStyle().ItemSpacing = ImVec2(6, 4);  // Reduce spacing between items ((8, 4) by default)
    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0.8, 0.8, 0.85, 1.0);  // Change text color
}


//////////////////////////////////////////////////////////////////////////
//    main(): here, we simply fill RunnerParams, then run the application
//////////////////////////////////////////////////////////////////////////
int main(int, char**)
{
    //#############################################################################################
    // Part 1: Define the application state, fill the status and menu bars, load additional font
    //#############################################################################################

    // Our application state
    AppState appState;

    // Hello ImGui params (they hold the settings as well as the Gui callbacks)
    HelloImGui::RunnerParams runnerParams;
    runnerParams.appWindowParams.windowTitle = "Docking Demo";
    runnerParams.imGuiWindowParams.menuAppTitle = "Docking Demo";
    runnerParams.appWindowParams.windowGeometry.size = {1200, 1000};
    runnerParams.appWindowParams.restorePreviousGeometry = true;

    // Our application uses a borderless window, but is movable/resizable
    runnerParams.appWindowParams.borderless = true;
    runnerParams.appWindowParams.borderlessMovable = true;
    runnerParams.appWindowParams.borderlessResizable = true;
    runnerParams.appWindowParams.borderlessClosable = true;

    // Load additional font
    runnerParams.callbacks.LoadAdditionalFonts = [&appState]() { LoadFonts(appState); };

    //
    // Status bar
    //
    // We use the default status bar of Hello ImGui
    runnerParams.imGuiWindowParams.showStatusBar = true;
    // uncomment next line in order to hide the FPS in the status bar
    // runnerParams.imGuiWindowParams.showStatusFps = false;
    runnerParams.callbacks.ShowStatus = [&appState]() { StatusBarGui(appState); };

    //
    // Menu bar
    //
    // Here, we fully customize the menu bar:
    // by setting `showMenuBar` to true, and `showMenu_App` and `showMenu_View` to false,
    // HelloImGui will display an empty menu bar, which we can fill with our own menu items via the callback `ShowMenus`
    runnerParams.imGuiWindowParams.showMenuBar = true;
    runnerParams.imGuiWindowParams.showMenu_App = false;
    runnerParams.imGuiWindowParams.showMenu_View = false;
    // Inside `ShowMenus`, we can call `HelloImGui::ShowViewMenu` and `HelloImGui::ShowAppMenu` if desired
    runnerParams.callbacks.ShowMenus = [&runnerParams]() {ShowMenuGui(runnerParams);};
    // Optional: add items to Hello ImGui default App menu
    runnerParams.callbacks.ShowAppMenuItems = ShowAppMenuItems;

    //
    // Top and bottom toolbars
    //
    // toolbar options
    HelloImGui::EdgeToolbarOptions edgeToolbarOptions;
    edgeToolbarOptions.sizeEm = 2.5f;
    edgeToolbarOptions.WindowBg = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
    // top toolbar
    runnerParams.callbacks.AddEdgeToolbar(
        HelloImGui::EdgeToolbarType::Top,
        [&appState]() { ShowTopToolbar(appState); },
        edgeToolbarOptions
        );
    // right toolbar
    edgeToolbarOptions.WindowBg.w = 0.4f;
    runnerParams.callbacks.AddEdgeToolbar(
            HelloImGui::EdgeToolbarType::Right,
            [&appState]() { ShowRightToolbar(appState); },
            edgeToolbarOptions
            );

    //
    // Load user settings at `PostInit` and save them at `BeforeExit`
    //
    runnerParams.callbacks.PostInit = [&appState]   { LoadMyAppSettings(appState);};
    runnerParams.callbacks.BeforeExit = [&appState] { SaveMyAppSettings(appState);};

    // Change style
    runnerParams.callbacks.SetupImGuiStyle = SetupMyTheme;

    //###############################################################################################
    // Part 2: Define the application layout and windows
    //###############################################################################################

    // First, tell HelloImGui that we want full screen dock space (this will create "MainDockSpace")
    runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    // In this demo, we also demonstrate multiple viewports: you can drag windows outside out the main window in order to put their content into new native windows
    runnerParams.imGuiWindowParams.enableViewports = true;
    // Set the default layout
    runnerParams.dockingParams = CreateDefaultLayout(appState);
    // Add alternative layouts
    runnerParams.alternativeDockingLayouts = CreateAlternativeLayouts(appState);

    // uncomment the next line if you want to always start with the layout defined in the code
    //     (otherwise, modifications to the layout applied by the user layout will be remembered)
    // runnerParams.dockingParams.layoutCondition = HelloImGui::DockingLayoutCondition::ApplicationStart;

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
    //         "~/Library/Application Support" under macOS or iOS
    runnerParams.iniFolderType = HelloImGui::IniFolderType::AppUserConfigFolder;

    // runnerParams.iniFilename: this will be the name of the ini file in which the settings
    // will be stored.
    // In this example, the subdirectory Docking_Demo will be created under the folder defined
    // by runnerParams.iniFolderType.
    //
    // Note: if iniFilename is left empty, the name of the ini file will be derived
    // from appWindowParams.windowTitle
    runnerParams.iniFilename = "Docking_Demo/Docking_demo.ini";

    //###############################################################################################
    // Part 4: Run the app
    //###############################################################################################
    HelloImGui::DeleteIniSettings(runnerParams);

    // Optional: choose the backend combination
    // ----------------------------------------
    //runnerParams.platformBackendType = HelloImGui::PlatformBackendType::Sdl;
    //runnerParams.rendererBackendType = HelloImGui::RendererBackendType::Vulkan;

    HelloImGui::Run(runnerParams); // Note: with ImGuiBundle, it is also possible to use ImmApp::Run(...)

    return 0;
}
