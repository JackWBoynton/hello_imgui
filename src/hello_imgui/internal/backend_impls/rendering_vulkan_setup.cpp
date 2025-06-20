#ifdef HELLOIMGUI_HAS_VULKAN

#include "rendering_vulkan.h"
#include "imgui_impl_vulkan.h"

#ifdef HELLOIMGUI_USE_GLFW3
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan.h>
#include "hello_imgui/hello_imgui_logger.h"

#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

namespace HelloImGui::VulkanSetup
{

void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    std::fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        std::abort();
}

#ifdef IMGUI_VULKAN_DEBUG_REPORT
VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
    VkDebugReportFlagsEXT        flags,
    VkDebugReportObjectTypeEXT   objectType,
    uint64_t                     object,
    size_t                       location,
    int32_t                      messageCode,
    const char*                  pLayerPrefix,
    const char*                  pMessage,
    void*                        pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix;
    std::fprintf(stderr, "[vulkan] Debug report from ObjectType %u:\n%s\n\n",
                 static_cast<unsigned>(objectType), pMessage);
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const auto& p : properties)
        if (std::strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

VkPhysicalDevice SetupVulkan_SelectPhysicalDevice()
{
    auto& g = HelloImGui::GetVulkanGlobals();

    uint32_t gpu_count = 0;
    VkResult err = vkEnumeratePhysicalDevices(g.Instance, &gpu_count, nullptr);
    check_vk_result(err);
    IM_ASSERT(gpu_count > 0);

    ImVector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    err = vkEnumeratePhysicalDevices(g.Instance, &gpu_count, gpus.Data);
    check_vk_result(err);

    // Prefer discrete GPU
    for (auto& device : gpus)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }
    // Fallback
    return gpus[0];
}

void SetupVulkan(ImVector<const char*> instance_extensions)
{
    auto& g = HelloImGui::GetVulkanGlobals();
    VkResult err;

    // -- Create Vulkan Instance --
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate instance extensions
        uint32_t ext_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
        ImVector<VkExtensionProperties> ext_props;
        ext_props.resize(ext_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, ext_props.Data);
        check_vk_result(err);

        if (IsExtensionAvailable(ext_props, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    #ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(ext_props, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    #endif

        // Enumerate & filter validation layer
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layerProps(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layerProps.data());

        const char* desiredLayer = "VK_LAYER_KHRONOS_validation";
        bool haveValidation = false;
        for (auto& lp : layerProps)
        {
            if (std::strcmp(lp.layerName, desiredLayer) == 0)
            {
                haveValidation = true;
                break;
            }
        }

    #ifdef IMGUI_VULKAN_DEBUG_REPORT
        if (haveValidation)
        {
            create_info.enabledLayerCount   = 1;
            create_info.ppEnabledLayerNames = &desiredLayer;
            instance_extensions.push_back("VK_EXT_debug_report");
        }
        else
        {
            std::fprintf(stderr, "[vulkan] Validation layer not found; skipping it.\n");
            create_info.enabledLayerCount   = 0;
            create_info.ppEnabledLayerNames = nullptr;
        }
    #else
        create_info.enabledLayerCount   = 0;
        create_info.ppEnabledLayerNames = nullptr;
    #endif

        // Setup extensions
        create_info.enabledExtensionCount   = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;

        // Create instance
        err = vkCreateInstance(&create_info, g.Allocator, &g.Instance);
        check_vk_result(err);

    }

    // -- Select Physical Device --
    g.PhysicalDevice = SetupVulkan_SelectPhysicalDevice();

    // -- Select Graphics Queue --
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(g.PhysicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> queues(count);
        vkGetPhysicalDeviceQueueFamilyProperties(g.PhysicalDevice, &count, queues.data());
        for (uint32_t i = 0; i < count; ++i)
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                g.QueueFamily = i;
                break;
            }
        IM_ASSERT(g.QueueFamily != UINT32_MAX);
    }

    // -- Create Logical Device --
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        uint32_t dev_ext_count = 0;
        vkEnumerateDeviceExtensionProperties(g.PhysicalDevice, nullptr, &dev_ext_count, nullptr);
        ImVector<VkExtensionProperties> dev_ext_props;
        dev_ext_props.resize(dev_ext_count);
        vkEnumerateDeviceExtensionProperties(g.PhysicalDevice, nullptr, &dev_ext_count, dev_ext_props.Data);

    #ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(dev_ext_props, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    #endif

        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = g.QueueFamily;
        queue_info.queueCount       = 1;
        queue_info.pQueuePriorities = &priority;

        VkDeviceCreateInfo di = {};
        di.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        di.queueCreateInfoCount    = 1;
        di.pQueueCreateInfos       = &queue_info;
        di.enabledExtensionCount   = (uint32_t)device_extensions.Size;
        di.ppEnabledExtensionNames = device_extensions.Data;

        err = vkCreateDevice(g.PhysicalDevice, &di, g.Allocator, &g.Device);
        check_vk_result(err);
        vkGetDeviceQueue(g.Device, g.QueueFamily, 0, &g.Queue);
    }

    // -- Create Descriptor Pool --
    {
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, g.PoolCreateInfo_PoolSizes }
        };
        VkDescriptorPoolCreateInfo dpi = {};
        dpi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpi.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        dpi.maxSets       = g.PoolCreateInfo_MaxSets;
        dpi.poolSizeCount = 1;
        dpi.pPoolSizes    = pool_sizes;
        err = vkCreateDescriptorPool(g.Device, &dpi, g.Allocator, &g.DescriptorPool);
        check_vk_result(err);
    }
}

void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    auto& g = HelloImGui::GetVulkanGlobals();
    wd->Surface = surface;

    // Check WSI support
    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(g.PhysicalDevice, g.QueueFamily, wd->Surface, &supported);
    IM_ASSERT(supported == VK_TRUE);

    // -- Fixed: use real arrays instead of compound literals --
    const VkFormat requestSurfaceImageFormats[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM
    };
    const uint32_t requestSurfaceFormatCount = 
        sizeof(requestSurfaceImageFormats) / sizeof(requestSurfaceImageFormats[0]);

    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        g.PhysicalDevice,
        wd->Surface,
        requestSurfaceImageFormats,
        requestSurfaceFormatCount,
        VK_COLORSPACE_SRGB_NONLINEAR_KHR
    );

    VkPresentModeKHR presentModes[] = {
    #ifdef IMGUI_UNLIMITED_FRAME_RATE
        VK_PRESENT_MODE_MAILBOX_KHR,
        VK_PRESENT_MODE_IMMEDIATE_KHR,
    #endif
        VK_PRESENT_MODE_FIFO_KHR
    };
    const uint32_t presentModeCount = 
        sizeof(presentModes) / sizeof(presentModes[0]);

    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        g.PhysicalDevice,
        wd->Surface,
        presentModes,
        presentModeCount
    );

    IM_ASSERT(g.MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        g.Instance, g.PhysicalDevice, g.Device, wd,
        g.QueueFamily, g.Allocator,
        width, height,
        g.MinImageCount
    );
}

void CleanupVulkan()
{
    auto& g = HelloImGui::GetVulkanGlobals();
    vkDestroyDescriptorPool(g.Device, g.DescriptorPool, g.Allocator);

    vkDestroyDevice(g.Device, g.Allocator);
    vkDestroyInstance(g.Instance, g.Allocator);
}

void CleanupVulkanWindow()
{
    auto& g = HelloImGui::GetVulkanGlobals();
    ImGui_ImplVulkanH_DestroyWindow(g.Instance, g.Device, &g.ImGuiMainWindowData, g.Allocator);
}

void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    auto& g = HelloImGui::GetVulkanGlobals();
    VkResult err;
    auto& sem = wd->FrameSemaphores[wd->SemaphoreIndex];

    err = vkAcquireNextImageKHR(g.Device, wd->Swapchain, UINT64_MAX,
                                sem.ImageAcquiredSemaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g.SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);

    auto* fd = &wd->Frames[wd->FrameIndex];
    vkWaitForFences(g.Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
    check_vk_result(err);
    vkResetFences(g.Device, 1, &fd->Fence);

    vkResetCommandPool(g.Device, fd->CommandPool, 0);
    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(fd->CommandBuffer, &bi);
    check_vk_result(err);

    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass        = wd->RenderPass;
    rpbi.framebuffer       = fd->Framebuffer;
    rpbi.renderArea.extent = { (uint32_t)wd->Width, (uint32_t)wd->Height };
    rpbi.clearValueCount   = 1;
    rpbi.pClearValues      = &wd->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    vkCmdEndRenderPass(fd->CommandBuffer);
    vkEndCommandBuffer(fd->CommandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si = {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &sem.ImageAcquiredSemaphore;
    si.pWaitDstStageMask    = &waitStage;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &fd->CommandBuffer;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &sem.RenderCompleteSemaphore;

    err = vkQueueSubmit(g.Queue, 1, &si, fd->Fence);
    check_vk_result(err);
}

void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    auto& g = HelloImGui::GetVulkanGlobals();
    if (g.SwapChainRebuild)
        return;

    auto& sem = wd->FrameSemaphores[wd->SemaphoreIndex];
    VkPresentInfoKHR pi = {};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &sem.RenderCompleteSemaphore;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &wd->Swapchain;
    pi.pImageIndices      = &wd->FrameIndex;

    VkResult err = vkQueuePresentKHR(g.Queue, &pi);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g.SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
}

} // namespace HelloImGui::VulkanSetup

#endif // HELLOIMGUI_HAS_VULKAN
