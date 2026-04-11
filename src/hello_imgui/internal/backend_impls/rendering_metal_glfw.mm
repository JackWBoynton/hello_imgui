#ifdef HELLOIMGUI_HAS_METAL
#ifdef HELLOIMGUI_USE_GLFW3

#include "rendering_metal.h"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include "imgui_impl_metal.h"
#include <array>

#include "hello_imgui/hello_imgui.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "imgui_impl_glfw.h"

// Pass-through views: hitTest returns nil so all input events reach GLFW's content view.
@interface HIMPassthroughView : NSView
@end
@implementation HIMPassthroughView
- (NSView*)hitTest:(NSPoint)point { return nil; }
@end

@interface HIMPassthroughEffectView : NSVisualEffectView
@end
@implementation HIMPassthroughEffectView
- (NSView*)hitTest:(NSPoint)point { return nil; }
@end


namespace HelloImGui
{

    GlfwMetalGlobals& GetGlfwMetalGlobals()
    {
            static GlfwMetalGlobals sGlfwMetalGlobals;
            return sGlfwMetalGlobals;
    }

    // Below is implementation of RenderingCallbacks_LinkWindowingToRenderingBackend
    void PrepareGlfwForMetal(GLFWwindow* glfwWindow, const RendererBackendOptions& rendererBackendOptions)
    {
        auto& gMetalGlobals = GetMetalGlobals();
        auto& gGlfwMetalGlobals = GetGlfwMetalGlobals();
        {
            gGlfwMetalGlobals.glfwWindow = glfwWindow;
            gMetalGlobals.mtlDevice = MTLCreateSystemDefaultDevice();
            gMetalGlobals.mtlCommandQueue = [gMetalGlobals.mtlDevice newCommandQueue];
        }
        {
            ImGui_ImplGlfw_InitForOther(gGlfwMetalGlobals.glfwWindow, true);
            ImGui_ImplMetal_Init(gMetalGlobals.mtlDevice);

            NSWindow* nswin = glfwGetCocoaWindow(gGlfwMetalGlobals.glfwWindow);

            // --- Transparent titlebar ---
            if (rendererBackendOptions.transparentTitleBar)
            {
                nswin.titleVisibility = NSWindowTitleHidden;
                nswin.titlebarAppearsTransparent = YES;
                nswin.styleMask |= NSWindowStyleMaskFullSizeContentView;
            }

            // --- Native dark mode ---
            if (rendererBackendOptions.nativeDarkMode)
            {
                nswin.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
            }

            // --- Metal layer setup ---
            gMetalGlobals.caMetalLayer = [CAMetalLayer layer];
            gMetalGlobals.caMetalLayer.device = gMetalGlobals.mtlDevice;

            if (rendererBackendOptions.requestFloatBuffer)
            {
                gMetalGlobals.caMetalLayer.pixelFormat = MTLPixelFormatRGBA16Float;
                gMetalGlobals.caMetalLayer.wantsExtendedDynamicRangeContent = YES;
                gMetalGlobals.caMetalLayer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceExtendedSRGB);
            }
            else
                gMetalGlobals.caMetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

            // --- Vibrancy: insert NSVisualEffectView behind a semi-transparent Metal layer ---
            if (rendererBackendOptions.enableVibrancy)
            {
                NSView* contentView = nswin.contentView;
                [contentView setWantsLayer:YES];

                // Frosted glass behind everything
                HIMPassthroughEffectView* effectView =
                    [[HIMPassthroughEffectView alloc] initWithFrame:contentView.bounds];
                effectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
                effectView.state = NSVisualEffectStateActive;
                effectView.material = NSVisualEffectMaterialUnderWindowBackground;
                effectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
                [contentView addSubview:effectView];

                // Container view with reduced opacity so vibrancy shows through
                HIMPassthroughView* appContainer =
                    [[HIMPassthroughView alloc] initWithFrame:contentView.bounds];
                [appContainer setWantsLayer:YES];
                appContainer.layer.opaque = NO;
                appContainer.alphaValue = rendererBackendOptions.vibrancyContentOpacity;
                appContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

                // Metal rendering view inside the container
                HIMPassthroughView* metalView =
                    [[HIMPassthroughView alloc] initWithFrame:appContainer.bounds];
                metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
                gMetalGlobals.caMetalLayer.opaque = NO;
                gMetalGlobals.caMetalLayer.displaySyncEnabled = YES;
                metalView.layer = gMetalGlobals.caMetalLayer;
                [metalView setWantsLayer:YES];

                [appContainer addSubview:metalView];
                [contentView addSubview:appContainer];
            }
            else
            {
                // Standard path: Metal layer directly on contentView
                nswin.contentView.layer = gMetalGlobals.caMetalLayer;
                nswin.contentView.wantsLayer = YES;
            }

            gMetalGlobals.mtlRenderPassDescriptor = [MTLRenderPassDescriptor new];
        }
    }

    RenderingCallbacksPtr CreateBackendCallbacks_GlfwMetal()
    {
        auto callbacks = PrepareBackendCallbacksCommon();

        callbacks->Impl_GetFrameBufferSize = []
        {
            auto& gGlfwMetalGlobals = GetGlfwMetalGlobals();
            int width, height;
            glfwGetFramebufferSize(gGlfwMetalGlobals.glfwWindow, &width, &height);
            return ScreenSize{width, height};
        };

        return callbacks;
    }

} // namespace HelloImGui

#endif // HELLOIMGUI_USE_GLFW3
#endif // HELLOIMGUI_HAS_METAL