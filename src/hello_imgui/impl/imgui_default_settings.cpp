#include "hello_imgui/imgui_default_settings.h"
#include "hello_imgui/hello_imgui.h"
#include "hello_imgui/hello_imgui_assets.h"
#include "hello_imgui/hello_imgui_font.h"

namespace HelloImGui
{

namespace ImGuiDefaultSettings
{

    void LoadDefaultFont_WithFontAwesomeIcons()
    {
        auto runnerParams = HelloImGui::GetRunnerParams();
        auto defaultIconFont = runnerParams->callbacks.defaultIconFont;
        float fontSize = 24.f;

        std::string fontFilename = "fonts/jetbrains.ttf";

        if (!HelloImGui::AssetExists(fontFilename))
        {
            ImGui::GetIO().Fonts->AddFontDefault();
            return;
        }

        LoadFont(fontFilename, fontSize);

        if (defaultIconFont == HelloImGui::DefaultIconFont::NoIcons)
            return;

        std::string iconFontFile;
        HelloImGui::FontLoadingParams fontParams;
        if (defaultIconFont == HelloImGui::DefaultIconFont::FontAwesome4)
            iconFontFile = "fonts/fontawesome-webfont.ttf";
        else if (defaultIconFont == HelloImGui::DefaultIconFont::FontAwesome6)
            iconFontFile = "fonts/Font_Awesome_6_Free-Solid-900.otf";
        else if (defaultIconFont == HelloImGui::DefaultIconFont::VsCodeIcons)
        {
            iconFontFile = "fonts/vscode-codicons.ttf";
            fontParams.fontConfig.GlyphOffset = ImVec2(1, 4.2);
        }
        else
            return;

        if (!HelloImGui::AssetExists(iconFontFile))
            return;

        fontParams.mergeToLastFont = true;
        LoadFont(iconFontFile, fontSize, fontParams);
    }

    void SetupDefaultImGuiConfig()
    {
        // Setup Dear ImGui context
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform
        // Windows

        // io.ConfigViewportsNoAutoMerge = true;
        // io.ConfigViewportsNoTaskBarIcon = true;
    }

    void SetupDefaultImGuiStyle()
    {
        // Note: a theme was already applied via ImGuiWindowParams.tweakedTheme

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows
        // can look identical to regular ones.
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    }

}  // namespace ImGuiDefaultSettings
}  // namespace HelloImGui
