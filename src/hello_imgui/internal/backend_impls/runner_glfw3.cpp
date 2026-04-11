#ifdef HELLOIMGUI_USE_GLFW3

#ifdef HELLOIMGUI_HAS_OPENGL
//#include "hello_imgui/hello_imgui_include_opengl.h"
#include "imgui_impl_opengl3.h"
#include "opengl_setup_helper/opengl_setup_glfw.h"
#include "opengl_setup_helper/opengl_screenshot.h"
#include "rendering_opengl3.h"
#endif
#ifdef HELLOIMGUI_HAS_METAL
#include "rendering_metal.h"
#endif
#ifdef HELLOIMGUI_HAS_VULKAN
#include "rendering_vulkan.h"
#endif

// DirectX unsupported with Glfw
#ifdef HELLOIMGUI_HAS_DIRECTX11
#include "rendering_dx11.h"
#endif
//#ifdef HELLOIMGUI_HAS_DIRECTX12
//#include "rendering_dx12.h"
//#endif

#include "hello_imgui/hello_imgui.h"
#include "stb_image.h"

#include "backend_window_helper/glfw_window_helper.h"
#include "runner_glfw3.h"
#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include <imgui.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <windowsx.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

namespace HelloImGui { namespace WindowsCustomFrame {

    // Stored options for the WndProc to reference
    static float sTitleBarHeight = 0.f;
    static WNDPROC sOriginalWndProc = nullptr;

    static int GetResizeBorderWidth() {
        return GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    }

    static LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Let DWM handle its messages first (shadow, etc.)
        LRESULT dwmResult = 0;
        if (DwmDefWindowProc(hWnd, msg, wParam, lParam, &dwmResult))
            return dwmResult;

        switch (msg) {
        case WM_ACTIVATE: {
            MARGINS margins = {0, 0, 1, 0};
            DwmExtendFrameIntoClientArea(hWnd, &margins);
            return 0;
        }

        case WM_NCCALCSIZE: {
            if (wParam == TRUE) {
                // Return 0 to remove the standard non-client frame.
                // When maximized, compensate so the window doesn't cover the taskbar.
                auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                if (IsZoomed(hWnd)) {
                    HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO mi = {sizeof(mi)};
                    if (GetMonitorInfoW(monitor, &mi))
                        params->rgrc[0] = mi.rcWork;
                }
                return 0;
            }
            break;
        }

        case WM_NCHITTEST: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT rc;
            GetWindowRect(hWnd, &rc);

            const int border = GetResizeBorderWidth();
            const int titlebar = static_cast<int>(sTitleBarHeight);
            const bool maximized = IsZoomed(hWnd);

            // Resize borders (disabled when maximized)
            if (!maximized) {
                if (pt.y >= rc.top && pt.y < rc.top + border) {
                    if (pt.x < rc.left + border) return HTTOPLEFT;
                    if (pt.x >= rc.right - border) return HTTOPRIGHT;
                    return HTTOP;
                }
                if (pt.y >= rc.bottom - border) {
                    if (pt.x < rc.left + border) return HTBOTTOMLEFT;
                    if (pt.x >= rc.right - border) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                if (pt.x >= rc.left && pt.x < rc.left + border) return HTLEFT;
                if (pt.x >= rc.right - border) return HTRIGHT;
            }

            // Titlebar area: drag-to-move (skip the right ~138px for caption buttons)
            if (titlebar > 0 && pt.y < rc.top + titlebar) {
                if (pt.x >= rc.right - 138)
                    return HTCLIENT;  // Caption buttons zone — let ImGui handle
                return HTCAPTION;
            }

            return HTCLIENT;
        }

        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = {sizeof(mi)};
            if (GetMonitorInfoW(monitor, &mi)) {
                mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
                mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
                mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
            }
            return 0;
        }
        }

        return CallWindowProcW(sOriginalWndProc, hWnd, msg, wParam, lParam);
    }

    static void Install(HWND hWnd, float titleBarHeight) {
        sTitleBarHeight = titleBarHeight;
        sOriginalWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CustomWndProc)));

        // Extend DWM frame (1px top for shadow)
        MARGINS margins = {0, 0, 1, 0};
        DwmExtendFrameIntoClientArea(hWnd, &margins);

        // Force a WM_NCCALCSIZE so the custom frame takes effect immediately
        RECT rc;
        GetWindowRect(hWnd, &rc);
        SetWindowPos(hWnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

}} // namespace HelloImGui::WindowsCustomFrame
#endif // _WIN32

#include <thread>
#include <cstdlib>
//extern char **environ;



namespace HelloImGui
{
#ifdef HELLOIMGUI_HAS_OPENGL
    BackendApi::OpenGlSetupGlfw gOpenGlSetupGlfw;
#endif

    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    RunnerGlfw3::RunnerGlfw3(RunnerParams & runnerParams)
        : AbstractRunner(runnerParams)
    {
        mBackendWindowHelper = std::make_unique<BackendApi::GlfwWindowHelper>();
    }

    void RunnerGlfw3::Impl_InitPlatformBackend()
    {
        glfwSetErrorCallback(glfw_error_callback);
        #ifdef __APPLE__
            // prevent glfw from changing the current dir on macOS.
            // This glfw behaviour is for Mac only, and interferes with our multiplatform assets handling
            glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
        #endif
        bool glfwInitSuccess = glfwInit();
        (void) glfwInitSuccess;
        IM_ASSERT(glfwInitSuccess);
    }

    void RunnerGlfw3::Impl_CreateWindow(std::function<void()> renderCallbackDuringResize)
    {
        BackendApi::BackendOptions backendOptions;
        backendOptions.rendererBackendType = params.rendererBackendType;
        mWindow = mBackendWindowHelper->CreateWindow(params.appWindowParams, backendOptions, renderCallbackDuringResize);
        params.backendPointers.glfwWindow = mWindow;

#ifdef _WIN32
        // Apply Windows-specific DWM attributes after window creation
        {
            HWND hwnd = glfwGetWin32Window((GLFWwindow*)mWindow);
            if (hwnd)
            {
                const auto& opts = params.rendererBackendOptions;

                // Dark mode titlebar
                if (opts.nativeDarkMode)
                {
                    BOOL useDark = TRUE;
                    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
                }

                // Mica backdrop (Win11 22621+, no-op on older Windows)
                if (opts.enableVibrancy)
                {
                    int backdropType = 2;  // DWM_SYSTEMBACKDROP_TYPE_MICA
                    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

                    // Extend frame into client area so the Mica backdrop is visible
                    MARGINS margins = {-1, -1, -1, -1};
                    DwmExtendFrameIntoClientArea(hwnd, &margins);
                }

                // Custom frameless window (remove standard caption, add resize/drag handling)
                if (opts.customWindowFrame)
                {
                    WindowsCustomFrame::Install(hwnd, opts.titleBarHeight);
                }
            }
        }
#endif
    }

    void RunnerGlfw3::Impl_PollEvents()
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants
        // to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
        // application. Generally you may always pass all inputs to dear imgui, and hide them from your
        // application based on those two flags.
        glfwPollEvents();
        bool exitRequired = (glfwWindowShouldClose((GLFWwindow *)mWindow) != 0);
        if (exitRequired)
            params.appShallExit = true;
    }

    void RunnerGlfw3::Impl_NewFrame_PlatformBackend() { ImGui_ImplGlfw_NewFrame(); }

    void RunnerGlfw3::Impl_UpdateAndRenderAdditionalPlatformWindows()
    {
        #ifdef HELLOIMGUI_HAS_OPENGL
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
        #endif
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        #ifdef HELLOIMGUI_HAS_OPENGL
            glfwMakeContextCurrent(backup_current_context);
        #endif
    }


    static void _ForceHideWindow_IfRunningInside_Notebook_MacOs_WithDebugger(GLFWwindow *window)
    {
    #ifdef __APPLE__
        // For some reason the window did sometimes stay opened (although inactive)
        // when used under python / notebook / macOS / vscode, even after a call to Impl_Cleanup()

        // macOS Cocoa's window management is asynchronous. When running under Python
        // debuggers (PyCharm, VS Code) or Jupyter notebooks, the event loop interaction
        // causes glfwHideWindow() to not complete before glfwDestroyWindow() is called.
        // This results in "ghost" windows that remain visible but inactive.
        //
        // The workaround: explicitly hide the window and wait for Cocoa's event processing
        // to complete before destroying it. All three steps are required:
        // 1. glfwHideWindow() - initiates the async hide operation
        // 2. glfwPollEvents() - processes Cocoa's event queue
        // 3. Time delay - gives Cocoa's main thread time to execute the hide

        // This will trigger only when running inside in a notebook with debugger (such as vscode)
        // (and is not needed inside browser based jupyter lab or notebook)

        // Debug code that was used for env detection: print all environment variables
        // for (char **env = environ; *env != nullptr; env++) {
        //     if (strstr(*env, "PY") || strstr(*env, "JUPYTER") || strstr(*env, "IPYTHON"))
        //         printf("Found: %s\n", *env);
        // }
        // Found: PYTHONUNBUFFERED=1
        // Found: PYTHONIOENCODING=utf-8
        // Found: PYDEVD_IPYTHON_COMPATIBLE_DEBUGGING=1
        // Found: PYTHON_FROZEN_MODULES=on
        // Found: PYDEVD_USE_FRAME_EVAL=NO

        bool shouldForceHideWindow = getenv("PYDEVD_IPYTHON_COMPATIBLE_DEBUGGING") != nullptr ||
               getenv("PYDEVD_USE_FRAME_EVAL") != nullptr;
        if (shouldForceHideWindow)
        {
            if (window)
            {
                glfwHideWindow(window);
                for (int i = 0; i < 5; i++)
                {
                    glfwPollEvents();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
#endif
    }

    void RunnerGlfw3::Impl_Cleanup()
    {
        _ForceHideWindow_IfRunningInside_Notebook_MacOs_WithDebugger((GLFWwindow *)mWindow);

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow((GLFWwindow *)mWindow);
        glfwTerminate();
    }

    void RunnerGlfw3::Impl_SwapBuffers()
    {
        // Note: call of RenderingCallbacks_Impl_SwapBuffers
        #ifdef HELLOIMGUI_HAS_OPENGL
        #ifndef __EMSCRIPTEN__  // glfwSwapBuffers is not implemented on emscripten/browser (pongasoft)
            if (params.rendererBackendType == RendererBackendType::OpenGL3)
                glfwSwapBuffers((GLFWwindow *)mWindow);
        #endif
        #endif
        #ifdef HELLOIMGUI_HAS_METAL
            if (params.rendererBackendType == RendererBackendType::Metal)
                SwapMetalBuffers();
        #endif
        #ifdef HELLOIMGUI_HAS_VULKAN
            if (params.rendererBackendType == RendererBackendType::Vulkan)
                SwapVulkanBuffers();
        #endif
        #ifdef HELLOIMGUI_HAS_DIRECTX11
            if (params.rendererBackendType == RendererBackendType::DirectX11)
                SwapDx11Buffers();
        #endif
        #ifdef HELLOIMGUI_HAS_DIRECTX12
            // if (params.rendererBackendType == RendererBackendType::DirectX12)
            //     SwapDx12Buffers();
        #endif
    }

    void RunnerGlfw3::Impl_SetWindowIcon()
    {
#ifndef __APPLE__
        std::string iconFile = "app_settings/icon.png";
        if (!HelloImGui::AssetExists(iconFile))
            return;

        auto imageAsset = HelloImGui::LoadAssetFileData(iconFile.c_str());
        int width, height, channels;
        unsigned char* image = stbi_load_from_memory(
            (stbi_uc *)imageAsset.data, (int)imageAsset.dataSize , &width, &height, &channels, 4); // force RGBA channels

        if (image)
        {
            GLFWimage icons[1];
            icons[0].width = width;
            icons[0].height = height;
            icons[0].pixels = image; // GLFWImage expects an array of pixels (unsigned char *)

            glfwSetWindowIcon((GLFWwindow*)mWindow, 1, icons);

            stbi_image_free(image);
        }
        else
            HIMG_LOG("RunnerGlfwOpenGl3::Impl_SetWindowIcon: Failed to load window icon: " + iconFile);
        HelloImGui::FreeAssetFileData(&imageAsset);
#endif
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Link with Rendering Backends (OpenGL, Vulkan, ...)
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
#if defined(HELLOIMGUI_HAS_OPENGL)
    void Impl_LinkPlatformAndRenderBackends_GlfwOpenGl(const RunnerGlfw3& runner)
    {
        ImGui_ImplGlfw_InitForOpenGL((GLFWwindow *)runner.mWindow, true);
        ImGui_ImplOpenGL3_Init(runner.Impl_GlslVersion().c_str());
    }

    void RunnerGlfw3::Impl_CreateGlContext()
    {
        glfwMakeContextCurrent((GLFWwindow *) mWindow); // OpenGl!
    }

    void RunnerGlfw3::Impl_Select_Gl_Version()
    {
        auto openGlOptions = gOpenGlSetupGlfw.OpenGlOptionsWithUserSettings();
        gOpenGlSetupGlfw.SelectOpenGlVersion(openGlOptions);
    }

    std::string RunnerGlfw3::Impl_GlslVersion() const
    {
        return std::string("#version ") + gOpenGlSetupGlfw.OpenGlOptionsWithUserSettings().GlslVersion;
    }

    void RunnerGlfw3::Impl_InitGlLoader() { gOpenGlSetupGlfw.InitGlLoader(); }
#endif // HELLOIMGUI_HAS_OPENGL

#if defined(HELLOIMGUI_HAS_METAL)
    void Impl_LinkPlatformAndRenderBackends_GlfwMetal(const RunnerGlfw3& runner)
    {
        PrepareGlfwForMetal((GLFWwindow *) runner.mWindow, runner.params.rendererBackendOptions);
    }
#endif

#if defined(HELLOIMGUI_HAS_VULKAN)
    void Impl_LinkPlatformAndRenderBackends_GlfwVulkan(const RunnerGlfw3& runner)
    {
        // Below, call of RenderingCallbacks_LinkWindowingToRenderingBackend
        PrepareGlfwForVulkan((GLFWwindow *) runner.mWindow);
    }
#endif // HELLOIMGUI_HAS_VULKAN

#if defined(HELLOIMGUI_HAS_DIRECTX11)
    void Impl_LinkPlatformAndRenderBackends_GlfwDirectX11(const RunnerGlfw3& runner)
    {
        // Below, call of RenderingCallbacks_LinkWindowingToRenderingBackend
        PrepareGlfwForDx11((GLFWwindow *) runner.mWindow);
    }
#endif

#if defined(HELLOIMGUI_HAS_DIRECTX12)
//    void Impl_LinkPlatformAndRenderBackends_GlfwDirectX12(const RunnerGlfw3& runner)
//    {
//        // Below, call of RenderingCallbacks_LinkWindowingToRenderingBackend
//        PrepareGlfwForDx12((GLFWwindow *) runner.mWindow);
//    }
#endif

    void RunnerGlfw3::Impl_LinkPlatformAndRenderBackends()
    {
        auto& self = *this;
        if (params.rendererBackendType == RendererBackendType::OpenGL3)
        {
            #ifdef HELLOIMGUI_HAS_OPENGL
                Impl_LinkPlatformAndRenderBackends_GlfwOpenGl(self);
            #else
                IM_ASSERT(false && "OpenGL3 not supported");
            #endif
        }
        else if(params.rendererBackendType == RendererBackendType::Metal)
        {
            #ifdef HELLOIMGUI_HAS_METAL
                Impl_LinkPlatformAndRenderBackends_GlfwMetal(self);
            #else
                IM_ASSERT(false && "Metal not supported");
            #endif
        }
        else if(params.rendererBackendType == RendererBackendType::Vulkan)
        {
            #ifdef HELLOIMGUI_HAS_VULKAN
                Impl_LinkPlatformAndRenderBackends_GlfwVulkan(self);
            #else
                IM_ASSERT(false && "Vulkan not supported");
            #endif
        }
        else if(params.rendererBackendType == RendererBackendType::DirectX11)
        {
            #ifdef HELLOIMGUI_HAS_DIRECTX11
                Impl_LinkPlatformAndRenderBackends_GlfwDirectX11(self);
            #else
                IM_ASSERT(false && "DirectX11 not supported");
            #endif
        }
        else if(params.rendererBackendType == RendererBackendType::DirectX12)
        {
            #ifdef HELLOIMGUI_HAS_DIRECTX12
                // Impl_LinkPlatformAndRenderBackends_GlfwDirectX12(self);
                IM_ASSERT(false && "The combination Glfw + DirectX12 is not supported");
            #else
                IM_ASSERT(false && "DirectX12 not supported");
            #endif
        }
    }

    void RunnerGlfw3::Impl_ApplyVsyncSetting()
    {
        #ifdef HELLOIMGUI_HAS_OPENGL
        if (params.fpsIdling.vsyncToMonitor)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
        #endif
    }
}  // namespace HelloImGui
#endif  // #ifdef HELLOIMGUI_USE_GLFW3
