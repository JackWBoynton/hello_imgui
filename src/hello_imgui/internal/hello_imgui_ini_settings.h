#pragma once

#include "hello_imgui/screen_bounds.h"
#include "hello_imgui/hello_imgui.h"

#include <string>
#include <optional>


namespace HelloImGui
{
    struct RunnerParams;

    namespace HelloImGuiIniSettings
    {
        /*
         HelloImGui stores its settings in a file that groups several ini file, and looks like this

            ;;;<<<imgui>>>;;;
            [Window][Main window (title bar invisible)]
            Pos=0,0
            Size=1000,800
            Collapsed=0

            [Window][Debug##Default]
            Pos=60,60
            Size=400,400
            Collapsed=0

            [Docking][Data]

            ;;;<<<appWindow>>>;;;
            [WIN]
            WindowPosition=393,238
            WindowSize=971,691

            ;;;<<<otherIniInfo>>>;;;
            ...
         */

        struct IniFilenameOrContent;

        struct IniParts
        {
            struct IniPart
            {
                std::string Name;
                std::string Content;
            };

            bool           HasIniPart(const std::string& name);
            std::string    GetIniPart(const std::string& name);
            void           SetIniPart(const std::string& name, const std::string& content);

            static IniParts Load(IniFilenameOrContent filenameOrContent);
            void            Write(IniFilenameOrContent filenameOrContent);

            std::vector<IniPart> Parts;
        };
        IniParts SplitIniParts(const std::string& s);
        std::string JoinIniParts(const IniParts& parts);

        enum class IniFilenameOrContentMode {
            FILENAME,
            CONTENT,
        };

        struct IniFilenameOrContent
        {
            std::string filename;
            std::string content;
            IniParts    parts;
            IniFilenameOrContentMode mode;

            static IniFilenameOrContent FromFilename(std::string filename) {
                return IniFilenameOrContent{filename, "", {}, IniFilenameOrContentMode::FILENAME};
            }

            static IniFilenameOrContent FromContent(std::string content) {
                return IniFilenameOrContent{"", content, {}, IniFilenameOrContentMode::CONTENT};
            }
        };

        //
        // The settings below are global to the app
        //
        void SaveLastRunWindowBounds(IniFilenameOrContent filenameOrContent, const ScreenBounds& windowBounds);
        std::optional<ScreenBounds> LoadLastRunWindowBounds(IniFilenameOrContent filenameOrContent);
        std::optional<float> LoadLastRunDpiWindowSizeFactor(IniFilenameOrContent filenameOrContent);
        void SaveHelloImGuiMiscSettings(IniFilenameOrContent filenameOrContent, const RunnerParams& runnerParams);
        void LoadHelloImGuiMiscSettings(IniFilenameOrContent filenameOrContent, RunnerParams* inOutRunnerParams);

        //
        // The settings below are saved with values that can differ from layout to layout
        //
        void LoadImGuiSettings(IniFilenameOrContent filenameOrContent, const std::string& layoutName);
        void SaveImGuiSettings(IniFilenameOrContent filenameOrContent, const std::string& layoutName);
        bool HasUserDockingSettingsInImguiSettings(IniFilenameOrContent filenameOrContent, const DockingParams& dockingParams);

        void SaveDockableWindowsVisibility(IniFilenameOrContent filenameOrContent, const DockingParams& dockingParams);
        void LoadDockableWindowsVisibility(IniFilenameOrContent filenameOrContent, DockingParams* inOutDockingParams);

        //
        // User prefs
        //
        void        SaveUserPref(IniFilenameOrContent filenameOrContent, const std::string& userPrefName, const std::string& userPrefContent);
        std::string LoadUserPref(IniFilenameOrContent filenameOrContent, const std::string& userPrefName);

    }
}
