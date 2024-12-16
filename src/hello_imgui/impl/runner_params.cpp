#include "hello_imgui/runner_params.h"

#include "hello_imgui/internal/hello_imgui_ini_settings.h"

#include <filesystem>

namespace HelloImGui
{
    RunnerParams SimpleRunnerParams::ToRunnerParams() const
    {
        auto& self = *this;
        RunnerParams r;

        r.callbacks.ShowGui = self.guiFunction;

        r.appWindowParams.windowGeometry.size = self.windowSize;
        r.appWindowParams.windowGeometry.sizeAuto = self.windowSizeAuto;
        r.appWindowParams.restorePreviousGeometry = self.windowRestorePreviousGeometry;

        r.appWindowParams.windowTitle = self.windowTitle;

        r.fpsIdling.fpsIdle = self.fpsIdle;
        r.fpsIdling.enableIdling = self.enableIdling;

        return r;
    }


    // IniSettingsLocation returns the path to the ini file for the application settings.
    HelloImGuiIniSettings::IniFilenameOrContent IniSettingsLocation(const RunnerParams& runnerParams)
    {
        HelloImGuiIniSettings::IniFilenameOrContent iniFilenameOrContent;
        auto _getIniFileName = [& runnerParams]() -> std::string
        {
            auto _stringToSaneFilename=[](const std::string& s, const std::string& extension) -> std::string
            {
                std::string filenameSanitized;

                for (char c : s)
                {
                    if (isalnum(c))
                        filenameSanitized += c;
                    else
                        filenameSanitized += "_";
                }

                filenameSanitized += extension;
                return filenameSanitized;
            };

            if (! runnerParams.iniFilename.empty())
                return runnerParams.iniFilename;

            if (runnerParams.iniFilename_useAppWindowTitle && !runnerParams.appWindowParams.windowTitle.empty())
                return _stringToSaneFilename(runnerParams.appWindowParams.windowTitle, ".ini");

            return "imgui.ini";

        };

        auto mkdirToFilename = [](const std::string& filename) -> bool
        {
            std::filesystem::path p(filename);
            std::filesystem::path dir = p.parent_path();

            if (dir.empty())
                return true;

            if (std::filesystem::exists(dir))
                return std::filesystem::is_directory(dir) || std::filesystem::is_symlink(dir);

            return std::filesystem::create_directories(dir);
        };


        std::string iniFilename = _getIniFileName();
        std::string folder = HelloImGui::IniFolderLocation(runnerParams.iniFolderType);

        std::string iniFullFilename = folder.empty() ? iniFilename : folder + "/" + iniFilename;
        bool settingsDirIsAccessible = mkdirToFilename(iniFullFilename);
        IM_ASSERT(settingsDirIsAccessible);

        iniFilenameOrContent.filename = iniFullFilename;
        return iniFilenameOrContent;
    }

    // DeleteIniSettings deletes the ini file for the application settings.
    void DeleteIniSettings(const RunnerParams& runnerParams)
    {
        auto iniFilenameOrContent = IniSettingsLocation(runnerParams);

        if (iniFilenameOrContent.content.empty() && iniFilenameOrContent.filename.empty())
            return;

        if (!iniFilenameOrContent.filename.empty() && !std::filesystem::exists(iniFilenameOrContent.filename))
            return;

        if (!iniFilenameOrContent.content.empty())
        {
            iniFilenameOrContent.content.clear();
            return;
        }
        bool success = std::filesystem::remove(iniFilenameOrContent.filename);
        IM_ASSERT(success && "Failed to delete ini file %s");
    }

    // HasIniSettings returns true if the ini file for the application settings exists.
    bool HasIniSettings(const RunnerParams& runnerParams)
    {
        auto iniFilenameOrContent = IniSettingsLocation(runnerParams);

        if (iniFilenameOrContent.content.empty() && iniFilenameOrContent.filename.empty())
            return false;
        
        if (!iniFilenameOrContent.content.empty())
            return true;

        return std::filesystem::exists(iniFilenameOrContent.filename);
    }

}  // namespace HelloImGui