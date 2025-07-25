#pragma once
#include "imgui.h"
#include "hello_imgui/runner_callbacks.h"
#include <functional>
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <optional>
#include <stdio.h>

namespace HelloImGui
{
/**
@@md#DockingIntro

HelloImGui makes it easy to use dockable windows
 (based on ImGui [docking branch](https://github.com/ocornut/imgui/tree/docking)).

You can define several layouts and switch between them:  each layout which will remember
 the user modifications and the list of opened windows

HelloImGui will then provide a "View" menu with options to show/hide the dockable windows,
 restore the default layout, switch between layouts, etc.

![demo docking](https://traineq.org/ImGuiBundle/HelloImGuiLayout.gif)

* Source for this example: https://github.com/pthom/hello_imgui/tree/master/src/hello_imgui_demos/hello_imgui_demodocking
* [Video explanation on YouTube](https://www.youtube.com/watch?v=XKxmz__F4ow) (5 minutes)


The different available layouts are provided inside RunnerParams via the two members below:
```cpp
struct RunnerParams
{
    ...
    // default layout of the application
    DockingParams dockingParams;

    // optional alternative layouts
    std::vector<DockingParams> alternativeDockingLayouts;

    ...
};
```

And `DockingParams` contains members that define a layout:

```cpp
struct DockingParams
{
    // displayed name of the layout
    std::string layoutName = "Default";

    // list of splits
    // (which define spaces where the windows will be placed)
    std::vector<DockingSplit> dockingSplits;

    // list of windows
    // (with their gui code, and specifying in which space they will be placed)
    std::vector<DockableWindow> dockableWindows;

    ...
};
```

Inside DockingParams, the member `dockingSplits` specifies the layout, and the member `dockableWindows`
 specifies the list of dockable windows, along with their default location, and their code (given by lambdas).

 @@md


@@md#DockingExample

Below is an example that shows how to instantiate a layout:

1. First, define the docking splits:

```cpp
std::vector<HelloImGui::DockingSplit> CreateDefaultDockingSplits()
{
    //   Here, we want to split "MainDockSpace" (which is provided automatically)
    //   into three zones, like this:
    //    ___________________________________________
    //    |        |                                |
    //    | Command|                                |
    //    | Space  |    MainDockSpace               |
    //    |        |                                |
    //    |        |                                |
    //    |        |                                |
    //    -------------------------------------------
    //    |     MiscSpace                           |
    //    -------------------------------------------
    //

    // add a space named "MiscSpace" whose height is 25% of the app height.
    // This will split the preexisting default dockspace "MainDockSpace" in two parts.
    HelloImGui::DockingSplit splitMainMisc;
    splitMainMisc.initialDock = "MainDockSpace";
    splitMainMisc.newDock = "MiscSpace";
    splitMainMisc.direction = ImGuiDir_Down;
    splitMainMisc.ratio = 0.25f;

    // Then, add a space to the left which occupies a column
    // whose width is 25% of the app width
    HelloImGui::DockingSplit splitMainCommand;
    splitMainCommand.initialDock = "MainDockSpace";
    splitMainCommand.newDock = "CommandSpace";
    splitMainCommand.direction = ImGuiDir_Left;
    splitMainCommand.ratio = 0.25f;

    std::vector<HelloImGui::DockingSplit> splits {splitMainMisc, splitMainCommand};
    return splits;
}
```

2. Then, define the dockable windows:

```cpp
std::vector<HelloImGui::DockableWindow> CreateDockableWindows(AppState& appState)
{
    // A Command panel named "Commands" will be placed in "CommandSpace".
    // Its Gui is provided calls "CommandGui"
    HelloImGui::DockableWindow commandsWindow;
    commandsWindow.label = "Commands";
    commandsWindow.dockSpaceName = "CommandSpace";
    commandsWindow.GuiFunction = [&] { CommandGui(appState); };

    // A Log window named "Logs" will be placed in "MiscSpace".
    // It uses the HelloImGui logger gui
    HelloImGui::DockableWindow logsWindow;
    logsWindow.label = "Logs";
    logsWindow.dockSpaceName = "MiscSpace";
    logsWindow.GuiFunction = [] { HelloImGui::LogGui(); };

    ...
}
```

3. Finally, fill the RunnerParams

```cpp
HelloImGui::RunnerParams runnerParams;
runnerParams.imGuiWindowParams.defaultImGuiWindowType =
    HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;

runnerParams.dockingParams.dockingSplits = CreateDefaultDockingSplits();
runnerParams.dockingParams.dockableWindows = CreateDockableWindows();


HelloImGui::Run(runnerParams);
```

@@md
*/

/*****************************************************************************/

// A DockSpaceName is a simple string that identifies a zone on the screen
// where windows can be docked.
using DockSpaceName = std::string;


// @@md#DockingSplit

// DockingSplit is a struct that defines the way the docking splits should
// be applied on the screen in order to create new Dock Spaces.
// DockingParams contains a
//     vector<DockingSplit>
// in order to partition the screen at your will.
struct DockingSplit
{
    // `initialDock`: _DockSpaceName (aka string)_
    //  id of the space that should be split.
    //  At the start, there is only one Dock Space named "MainDockSpace".
    //  You should start by partitioning this space, in order to create a new dock space.
    DockSpaceName initialDock;

    // `newDock`: _DockSpaceName (aka string)_.
    //  id of the new dock space that will be created.
    DockSpaceName newDock;

    // `direction`: *ImGuiDir_*
    //  (enum with ImGuiDir_Down, ImGuiDir_Down, ImGuiDir_Left, ImGuiDir_Right)*
    //  Direction where this dock space should be created.
    ImGuiDir direction;

    // `ratio`: _float, default=0.25f_.
    //  Ratio of the initialDock size that should be used by the new dock space.
    float ratio = 0.25f;

    // `nodeFlags`: *ImGuiDockNodeFlags_ (enum)*.
    //  Flags to apply to the new dock space
    //  (enable/disable resizing, splitting, tab bar, etc.)
    ImGuiDockNodeFlags nodeFlags = ImGuiDockNodeFlags_None;

    // `defaultSize`: _ImVec2, default=(0.0f, 0.0f)_.
    // Set this size to a non-zero value if you want to ensure that the new dock space
    // has a specific size when it is created.
    ImVec2 defaultSize = ImVec2(10.0f, 10.0f);

    // Constructor
    DockingSplit(const DockSpaceName& initialDock_ = "",
                 const DockSpaceName& newDock_ = "",
                 ImGuiDir direction_ = ImGuiDir_Down,
                 float ratio_ = 0.25f,
                 ImGuiDockNodeFlags nodeFlags_ = ImGuiDockNodeFlags_None,
                 ImVec2 defaultSize_ = ImVec2(10.0f, 10.0f))
        : initialDock(initialDock_),
          newDock(newDock_),
          direction(direction_),
          ratio(ratio_),
          nodeFlags(nodeFlags_),
          defaultSize(defaultSize_)
    {
    }
};
// @@md

enum class DockingLayoutCondition
{
    FirstUseEver,
    ApplicationStart,
    Never
};

struct DockableWindow;

// @@md#DockingParams

// DockingParams contains all the settings concerning the docking:
//     - list of splits
//     - list of dockable windows
struct DockingParams
{
    // --------------- Main params -----------------------------

    // `dockingSplits`: _vector[DockingSplit]_.
    //  Defines the way docking splits should be applied on the screen
    //  in order to create new Dock Spaces
    std::vector<DockingSplit> dockingSplits;

    // `dockableWindows`: _vector[DockableWindow]_.
    //  List of the dockable windows, together with their Gui code
    std::vector<std::shared_ptr<DockableWindow>> dockableWindows;

    // `layoutName`: _string, default="default"_.
    //  Displayed name of the layout.
    //  Only used in advanced cases, when several layouts are available.
    std::string layoutName = "Default";


    // --------------- Options -----------------------------

    // `mainDockSpaceNodeFlags`: _ImGuiDockNodeFlags (enum),
    //      default=ImGuiDockNodeFlags_PassthruCentralNode_
    //  Flags to apply to the main dock space
    //  (enable/disable resizing, splitting, tab bar, etc.).
    //  Most flags are inherited by children dock spaces.
    //  You can also set flags for specific dock spaces via `DockingSplit.nodeFlags`
    ImGuiDockNodeFlags mainDockSpaceNodeFlags = ImGuiDockNodeFlags_PassthruCentralNode;


    // --------------- Layout handling -----------------------------

    // `layoutCondition`: _enum DockingLayoutCondition, default=FirstUseEver_.
    //  When to apply the docking layout. Choose between
    //      FirstUseEver (apply once, then keep user preference),
    //      ApplicationStart (always reapply at application start)
    //      Never
    DockingLayoutCondition layoutCondition = DockingLayoutCondition::FirstUseEver;

    // `layoutReset`: _bool, default=false_.
    //  Reset layout on next frame, i.e. drop the layout customizations which were
    //  applied manually by the user. layoutReset will be reset to false after this.
    bool layoutReset = false;


    // --------------- Helper Methods -----------------------------

    // `DockableWindow * dockableWindowOfName(const std::string & name)`:
    // returns a pointer to a dockable window
    std::shared_ptr<DockableWindow> dockableWindowOfName(const std::string& name);

    // `bool focusDockableWindow(const std::string& name)`:
    // will focus a dockable window (and make its tab visible if needed)
    bool focusDockableWindow(const std::string& windowName);

    // `optional<ImGuiID> dockSpaceIdFromName(const std::string& dockSpaceName)`:
    // returns the ImGuiID corresponding to the dockspace with this name
    std::optional<ImGuiID> dockSpaceIdFromName(const std::string& dockSpaceName);
};
// @@md

enum class DockableWindowAdditionState
{
    Waiting,
    AddedAsDummyToImGui,
    AddedToHelloImGui
};

// @@md#DockableWindow

// DockableWindow is a struct that represents a window that can be docked.
struct DockableWindow
{
    std::string label;
    DockSpaceName dockSpaceName;
    std::string category;
    VoidFunction GuiFunction = EmptyVoidFunction();
    VoidFunction customViewMenu = EmptyVoidFunction();
    std::function<void(std::shared_ptr<DockableWindow>&)> customTitleBarContextFunction = nullptr;
    bool isVisible = true;
    bool rememberIsVisible = true;
    bool canBeClosed = true;
    bool callBeginEnd = true;
    bool includeInViewMenu = true;
    ImGuiWindowFlags imGuiWindowFlags = 0;
    bool focusWindowAtNextFrame = false;
    ImVec2 windowSize = ImVec2(0.f, 0.f);
    ImGuiCond windowSizeCondition = ImGuiCond_FirstUseEver;
    ImVec2 windowPosition = ImVec2(0.f, 0.f);
    ImGuiCond windowPositionCondition = ImGuiCond_FirstUseEver;
    DockingParams dockingParams = {};
    bool wantsClose = false;
    bool justAdded = true;
    bool wantsAutoDock = true;
    DockableWindowAdditionState state = DockableWindowAdditionState::Waiting;
};
// @@md

namespace SplitIdsHelper
{
    inline DockSpaceName StripPrefix(const DockSpaceName& name)
    {
        auto pos = name.find("###");
        return (pos != DockSpaceName::npos) ? name.substr(pos) : name;
    }

    bool ContainsSplit(const DockSpaceName& dockSpaceName);
    ImGuiID GetSplitId(const DockSpaceName& dockSpaceName);
    void SetSplitId(const DockSpaceName& dockSpaceName, ImGuiID imguiId);

    std::string SaveSplitIds();
    void LoadSplitIds(const std::string& jsonStr);

}  // namespace SplitIdsHelper

}  // namespace HelloImGui
