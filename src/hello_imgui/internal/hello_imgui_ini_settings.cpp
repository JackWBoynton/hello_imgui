#include "hello_imgui/internal/hello_imgui_ini_settings.h"
#include "hello_imgui/internal/inicpp.h"
#include "hello_imgui/internal/functional_utils.h"
#include "hello_imgui_ini_settings.h"
#include "imgui_internal.h"


namespace HelloImGui
{
    // Encapsulated in docking_details.cpp
    namespace SplitIdsHelper
    {
        std::string SaveSplitIds();
        void LoadSplitIds(const std::string&);
    }


    std::string IntPairToString(std::array<int, 2> v);
    std::array<int, 2> StringToIntPair(const std::string& s);

    namespace HelloImGuiIniSettings
    {
        namespace details
        {
            std::string AlnumOrUnderscore(const std::string& s)
            {
                std::string r;
                for (char c : s)
                {
                    if (isalnum(c))
                        r += c;
                    else
                        r += "_";
                }
                return r;
            }

            std::string SanitizeIniNameOrCategory(const std::string& s)
            {
                std::stringstream ss;
                ss << ImHashStr(s.c_str()) << "_" << AlnumOrUnderscore(s);
                return ss.str();
            }

            bool _stringEndsWith(std::string const &fullString, std::string const &ending)
            {
                if (fullString.length() >= ending.length())
                    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
                else
                    return false;
            };

            bool _stringStartsWith(std::string const &fullString, std::string const &start)
            {
                if (fullString.length() >= start.length())
                    return (0 == fullString.compare(0, start.length(), start));
                else
                    return false;
            };

            std::string _windowNameInImguiIniLine(const std::string& line)
            {
                // Search for a line like
                //     [Window][Commands]
                // And return "Commands"
                std::string token = "[Window][";
                if (line.empty())
                    return "";
                if (line[line.size() - 1] != ']')
                    return "";
                if (!_stringStartsWith(line, token))
                    return "";

                std::string windowName = line.substr(token.size(), line.size() - token.size() - 1);
                return windowName;
            }

        }


        // Test if a line looks like an IniPart intro, e.g.:
        //      ;;;<<<imgui>>>;;;
        bool _IsIniPartName(const std::string& line)
        {
            return (line.find(";;;<<<")) == 0 && (line.rfind(">>>;;;") == line.size() - 6);
        }

        // Reads an IniPart intro name, e.g.:
        //      ;;;<<<imgui>>>;;;
        // =>
        //      imgui
        std::string _ReadIniPartName(const std::string& line)
        {
            IM_ASSERT(_IsIniPartName(line));
            IM_ASSERT(line.size() > 12);
            std::string name = line.substr(6, line.size() - 12);
            return name;
        }

        IniParts SplitIniParts(const std::string& s)
        {
            IniParts iniParts;
            std::optional<IniParts::IniPart> currentPart;

            auto lines = FunctionalUtils::split_lines(s);
            for (const std::string& line: lines)
            {
                if (_IsIniPartName(line))
                {
                    if (currentPart.has_value())
                        iniParts.Parts.push_back(*currentPart);

                    currentPart = IniParts::IniPart();
                    currentPart->Name = _ReadIniPartName(line);
                }
                else
                {
                    if (currentPart.has_value())
                        currentPart->Content += line + "\n";
                }
            }

            if (currentPart.has_value())
                iniParts.Parts.push_back(*currentPart);

            return iniParts;
        }

        std::string JoinIniParts(const IniParts& iniParts)
        {
            std::string r = ";;; !!! This configuration is handled by HelloImGui and stores several Ini Files, separated by markers like this:\n           ;;;<<<INI_NAME>>>;;;\n\n";
            for (const auto& iniPart: iniParts.Parts)
            {
                r += ";;;<<<" + iniPart.Name + ">>>;;;\n";
                r += iniPart.Content;
            }
            return r;
        }

        bool  IniParts::HasIniPart(const std::string& name)
        {
            bool r = false;
            for(const auto& iniPart: Parts)
                if (iniPart.Name == name)
                    return true;
            return false;
        }
        std::string IniParts::GetIniPart(const std::string& name)
        {
            IM_ASSERT(HasIniPart(name));
            for(const auto& iniPart: Parts)
                if (iniPart.Name == name)
                    return iniPart.Content;
            return "ERROR call HasIniPart before GetIniPart!!!!";
        }
        void IniParts::SetIniPart(const std::string& name, const std::string& content)
        {
            if (HasIniPart(name))
            {
                for(auto& iniPart: Parts)
                    if (iniPart.Name == name)
                        iniPart.Content = content;

            }
            else
                Parts.push_back(IniPart{name, content});
        }


        IniParts IniParts::Load(IniFilenameOrContent filenameOrContent)
        {
            std::string iniPartsContent;
            if (!filenameOrContent.filename.empty())
                iniPartsContent = FunctionalUtils::read_text_file_or_empty(filenameOrContent.filename);
            else
                iniPartsContent = filenameOrContent.content;
            auto iniParts = SplitIniParts(iniPartsContent);
            return iniParts;
        }

        void IniParts::Write(IniFilenameOrContent filenameOrContent)
        {
            std::string iniPartsContent = JoinIniParts(*this);
            if (!filenameOrContent.filename.empty())
                FunctionalUtils::write_text_file(filenameOrContent.filename, iniPartsContent);
            else
                filenameOrContent.content = iniPartsContent;
        }

        void SaveLastRunWindowBounds(IniFilenameOrContent filenameOrContent, const ScreenBounds& windowBounds)
        {
            auto& dpiAwareParams = HelloImGui::GetRunnerParams()->dpiAwareParams;
            IniParts iniParts = IniParts::Load(filenameOrContent);

            ini::IniFile iniFile;
            iniFile["AppWindow"]["WindowPosition"] = IntPairToString(windowBounds.position);
            iniFile["AppWindow"]["WindowSize"] = IntPairToString(windowBounds.size);
            iniFile["AppWindow"]["DpiWindowSizeFactor"] = dpiAwareParams.dpiWindowSizeFactor;
            std::string iniContent = iniFile.encode();

            iniParts.SetIniPart("AppWindow", iniContent);
            iniParts.Write(filenameOrContent);
        }

        std::optional<ScreenBounds> LoadLastRunWindowBounds(IniFilenameOrContent filenameOrContent)
        {
            IniParts iniParts = IniParts::Load(filenameOrContent);

            if (!iniParts.HasIniPart("AppWindow"))
                return std::nullopt;

            auto iniPartContent = iniParts.GetIniPart("AppWindow");
            ini::IniFile iniFile;
            try
            {
                iniFile.decode(iniPartContent);
            }
            catch(const std::exception&)
            {
                return std::nullopt;
            }

            ScreenBounds screenBounds;
            bool failed = false;

            if (iniFile.find("AppWindow") == iniFile.end())
                return std::nullopt;
            auto & appWindowSection = iniFile["AppWindow"];

            // Read Window Position
            {
                if (appWindowSection.find("WindowPosition") == appWindowSection.end())
                    return std::nullopt;
                auto strValue = appWindowSection["WindowPosition"].as<std::string>();
                auto intPair = StringToIntPair(strValue);
                if (intPair[0] >= 0)
                    screenBounds.position = intPair;
                else
                    failed = true;
            }
            // Read Window Size
            {
                if (appWindowSection.find("WindowSize") == appWindowSection.end())
                    return std::nullopt;
                auto strValue = appWindowSection["WindowSize"].as<std::string>();
                auto intPair = StringToIntPair(strValue);
                if (intPair[0] >= 0)
                    screenBounds.size = intPair;
                else
                    failed = true;
            }

            if (failed)
                return std::nullopt;
            else
                return screenBounds;

        }

        std::optional<float> LoadLastRunDpiWindowSizeFactor(IniFilenameOrContent filenameOrContent)
        {
            IniParts iniParts = IniParts::Load(filenameOrContent);

            if (!iniParts.HasIniPart("AppWindow"))
                return std::nullopt;

            auto iniPartContent = iniParts.GetIniPart("AppWindow");
            ini::IniFile iniFile;
            try
            {
                iniFile.decode(iniPartContent);
            }
            catch (const std::exception &)
            {
                return std::nullopt;
            }

            ScreenBounds screenBounds;
            bool failed = false;

            if (iniFile.find("AppWindow") == iniFile.end())
                return std::nullopt;
            auto &appWindowSection = iniFile["AppWindow"];

            if (appWindowSection.find("DpiWindowSizeFactor") != appWindowSection.end())
            {
                float dpiWindowSizeFactor_WhenSaved =
                    iniFile["AppWindow"]["DpiWindowSizeFactor"].as<float>();
                bool isDpiSane = (dpiWindowSizeFactor_WhenSaved >= 0.1f) &&
                                 (dpiWindowSizeFactor_WhenSaved <= 10.f);
                if (isDpiSane)
                    return dpiWindowSizeFactor_WhenSaved;
            }
            return std::nullopt;
        }


        void LoadImGuiSettings(IniFilenameOrContent filenameOrContent, const std::string& layoutName)
        {
            std::string iniPartName = "ImGui_" + details::SanitizeIniNameOrCategory(layoutName);

            IniParts iniParts = IniParts::Load(filenameOrContent);
            if (!iniParts.HasIniPart(iniPartName))
                return;
            auto imguiSettingsContent = iniParts.GetIniPart(iniPartName);
            ImGui::LoadIniSettingsFromMemory(imguiSettingsContent.c_str());
        }

        void SaveImGuiSettings(IniFilenameOrContent filenameOrContent, const std::string& layoutName)
        {
            std::string iniPartName = "ImGui_" + details::SanitizeIniNameOrCategory(layoutName);

            std::string imguiSettingsContent = ImGui::SaveIniSettingsToMemory();

            IniParts iniParts = IniParts::Load(filenameOrContent);
            iniParts.SetIniPart(iniPartName, imguiSettingsContent);
            iniParts.Write(filenameOrContent);
        }

        std::string SaveImGuiSettingsToString(const std::string& layoutName)
        {
            std::string iniPartName = "ImGui_" + details::SanitizeIniNameOrCategory(layoutName);
            std::string imguiSettingsContent = ImGui::SaveIniSettingsToMemory();
            return imguiSettingsContent;
        }

        static bool HasUserDockingSettingsInImGuiSettingsImpl(HelloImGuiIniSettings::IniParts& iniParts, const DockingParams& dockingParams) {
            std::string iniPartName = "ImGui_" + details::SanitizeIniNameOrCategory(dockingParams.layoutName);
            if (!iniParts.HasIniPart(iniPartName))
                return false;
            
            auto iniPartContent = iniParts.GetIniPart(iniPartName);
            std::stringstream ss(iniPartContent);

            std::vector<std::string> windowsWithSettings;
            std::string line;
            while (ss)
            {
                std::getline (ss, line);
                std::string w = details::_windowNameInImguiIniLine(line);
                if (!w.empty())
                    windowsWithSettings.push_back(w);
            }

            for (const auto& dockableWindow: dockingParams.dockableWindows)
            {
                if (
                std::find(windowsWithSettings.begin(), windowsWithSettings.end(), dockableWindow->label)
                == windowsWithSettings.end())
                {
                    return false;
                }
            }
            return true;
        }

        bool HasUserDockingSettingsInImguiSettings(IniFilenameOrContent filenameOrContent, const DockingParams& dockingParams)
        {

            auto iniParts = HelloImGuiIniSettings::IniParts::Load(filenameOrContent);
            return HasUserDockingSettingsInImGuiSettingsImpl(iniParts, dockingParams);
        }

        static void SaveDockableWindowsVisibilityRec(ini::IniFile& iniFile, const std::vector<DockableWindow*>& dockableWindows)
        {
            for (const auto& dockableWindow: dockableWindows)
            {
                if (dockableWindow->rememberIsVisible)
                {
                    std::string iniValueName = details::SanitizeIniNameOrCategory(dockableWindow->label);
                    iniFile["Visibility"][iniValueName] = dockableWindow->isVisible;
                }
                if (!dockableWindow->dockingParams.dockableWindows.empty())
                    SaveDockableWindowsVisibilityRec(iniFile, dockableWindow->dockingParams.dockableWindows);
            }
        }

        void SaveDockableWindowsVisibility(IniFilenameOrContent filenameOrContent, const DockingParams& dockingParams)
        {
            std::string iniPartName = "Layout_" + details::SanitizeIniNameOrCategory(dockingParams.layoutName);

            ini::IniFile iniFile;
            SaveDockableWindowsVisibilityRec(iniFile, dockingParams.dockableWindows);

            IniParts iniParts = IniParts::Load(filenameOrContent);
            iniParts.SetIniPart(iniPartName, iniFile.encode());
            iniParts.Write(filenameOrContent);
        }

        std::string SaveDockableWindowsVisibilityToString(const DockingParams &dockingParams) {
            std::string iniPartName = "Layout_" + details::SanitizeIniNameOrCategory(dockingParams.layoutName);

            ini::IniFile iniFile;
            SaveDockableWindowsVisibilityRec(iniFile, dockingParams.dockableWindows);
            
            IniParts iniParts;
            // TODO: this probably is missing information about the layout name
            iniParts.SetIniPart(iniPartName, iniFile.encode());

            return JoinIniParts(iniParts);
        }

        void LoadDockableWindowsVisibilityRec(ini::IniFile& iniFile, std::vector<DockableWindow*>& dockableWindows)
        {
            for (auto& dockableWindow: dockableWindows)
            {
                if (dockableWindow->rememberIsVisible)
                {
                    std::string iniValueName = details::SanitizeIniNameOrCategory(dockableWindow->label);
                    std::string boolString = iniFile["Visibility"][iniValueName].as<std::string>();

                    if (boolString == "true")
                        dockableWindow->isVisible = true;
                    if (boolString == "false")
                        dockableWindow->isVisible = false;
                }
                if (!dockableWindow->dockingParams.dockableWindows.empty())
                    LoadDockableWindowsVisibilityRec(iniFile, dockableWindow->dockingParams.dockableWindows);
            }
        }

        void LoadDockableWindowsVisibility(IniFilenameOrContent filenameOrContent, DockingParams* inOutDockingParams)
        {
            std::string iniPartName = "Layout_" + details::SanitizeIniNameOrCategory(inOutDockingParams->layoutName);

            IniParts iniParts = IniParts::Load(filenameOrContent);
            if (! iniParts.HasIniPart(iniPartName))
                return;

            ini::IniFile iniFile;
            iniFile.decode(iniParts.GetIniPart(iniPartName));
            LoadDockableWindowsVisibilityRec(iniFile, inOutDockingParams->dockableWindows);
        }

        void LoadSplitIds(IniFilenameOrContent filenameOrContent)
        {
            const std::string iniPartName = "SplitIds";
            std::string serialized = LoadUserPref(filenameOrContent, iniPartName);
            if (!serialized.empty())
                SplitIdsHelper::LoadSplitIds(serialized);
        }

        void SaveSplitIds(IniFilenameOrContent filenameOrContent)
        {
            const std::string iniPartName = "SplitIds";
            std::string serialized = SplitIdsHelper::SaveSplitIds();
            SaveUserPref(filenameOrContent, iniPartName, serialized);
        }

        void LoadHelloImGuiMiscSettings(IniFilenameOrContent filenameOrContent, RunnerParams* inOutRunnerParams)
        {
            std::string iniPartName = "HelloImGui_Misc";

            std::string layoutName = "";
            std::string themeName = "";
            {
                IniParts iniParts = IniParts::Load(filenameOrContent);
                if (iniParts.HasIniPart(iniPartName))
                {
                    ini::IniFile iniFile;
                    iniFile.decode(iniParts.GetIniPart(iniPartName));

                    if (inOutRunnerParams->rememberSelectedAlternativeLayout)
                        layoutName = iniFile["Layout"]["Name"].as<std::string>();
                    if (inOutRunnerParams->imGuiWindowParams.rememberTheme)
                        themeName = iniFile["Theme"]["Name"].as<std::string>();

                    if (inOutRunnerParams->imGuiWindowParams.rememberStatusBarSettings)
                    {
                        {
                            std::string s = iniFile["StatusBar"]["Show"].as<std::string>();
                            if (s == "true")
                                inOutRunnerParams->imGuiWindowParams.showStatusBar = true;
                            if (s == "false")
                                inOutRunnerParams->imGuiWindowParams.showStatusBar = false;
                        }
                        {
                            std::string s = iniFile["StatusBar"]["ShowFps"].as<std::string>();
                            if (s == "true")
                                inOutRunnerParams->imGuiWindowParams.showStatus_Fps = true;
                            if (s == "false")
                                inOutRunnerParams->imGuiWindowParams.showStatus_Fps = false;
                        }
                    }
                    if (inOutRunnerParams->fpsIdling.rememberEnableIdling)
                    {
                        std::string s = iniFile["Idling"]["EnableIdling"].as<std::string>();
                        if (s == "true")
                            inOutRunnerParams->fpsIdling.enableIdling = true;
                        if (s == "false")
                            inOutRunnerParams->fpsIdling.enableIdling = false;
                    }
                }
            }

            if (!themeName.empty())
            {
                auto theme = ImGuiTheme::ImGuiTheme_FromName(themeName.c_str());
                inOutRunnerParams->imGuiWindowParams.tweakedTheme.Theme = theme;
                ImGuiTheme::ApplyTheme(theme);
            }
            HelloImGui::SwitchLayout(layoutName);

            LoadSplitIds(filenameOrContent);
        }

        void SaveHelloImGuiMiscSettings(IniFilenameOrContent filenameOrContent, const RunnerParams& runnerParams)
        {
            std::string iniPartName = "HelloImGui_Misc";
            ini::IniFile iniFile;
            if (runnerParams.rememberSelectedAlternativeLayout)
                iniFile["Layout"]["Name"] = runnerParams.dockingParams.layoutName;
            if (runnerParams.imGuiWindowParams.rememberTheme)
                iniFile["Theme"]["Name"] = ImGuiTheme::ImGuiTheme_Name(runnerParams.imGuiWindowParams.tweakedTheme.Theme);
            if (runnerParams.imGuiWindowParams.rememberStatusBarSettings)
            {
                iniFile["StatusBar"]["Show"] = runnerParams.imGuiWindowParams.showStatusBar;
                iniFile["StatusBar"]["ShowFps"] = runnerParams.imGuiWindowParams.showStatus_Fps;
            }
            if (runnerParams.fpsIdling.rememberEnableIdling)
            {
                iniFile["Idling"]["EnableIdling"] = runnerParams.fpsIdling.enableIdling;
            }

            IniParts iniParts = IniParts::Load(filenameOrContent);
            iniParts.SetIniPart(iniPartName, iniFile.encode());
            iniParts.Write(filenameOrContent);

            SaveSplitIds(filenameOrContent);
        }


        void  SaveUserPref(IniFilenameOrContent filenameOrContent, const std::string& userPrefName, const std::string& userPrefContent)
        {
            IniParts iniParts = IniParts::Load(filenameOrContent);
            iniParts.SetIniPart(userPrefName, userPrefContent + "\n");
            iniParts.Write(filenameOrContent);
        }

        std::string LoadUserPref(IniFilenameOrContent filenameOrContent, const std::string& userPrefName)
        {
            IniParts iniParts = IniParts::Load(filenameOrContent);
            if (iniParts.HasIniPart(userPrefName))
            {
                std::string contentWithNewLine = iniParts.GetIniPart(userPrefName);
                return contentWithNewLine.substr(0, contentWithNewLine.size() - 1);
            }
            else
                return "";
        }


    } // namespace HelloImGuiIniSettings
} // namespace HelloImGui
