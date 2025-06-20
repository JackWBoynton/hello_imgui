#pragma once
#ifdef HELLOIMGUI_HAS_VULKAN

#include "image_abstract.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace HelloImGui
{
    struct ImageVulkan : public ImageAbstract
    {
        ImageVulkan() = default;
        ~ImageVulkan() override;

        ImTextureID TextureID() override;
        void _impl_StoreTexture(int width, int height, unsigned char* image_data_rgba) override;

        // Specific to Vulkan
        VkDescriptorSet DS                = VK_NULL_HANDLE;
        static constexpr int Channels     = 4; // We intentionally only support RGBA for now
        VkImageView     ImageView         = VK_NULL_HANDLE;
        VkImage         Image             = VK_NULL_HANDLE;
        VkDeviceMemory  ImageMemory       = VK_NULL_HANDLE;
        VkSampler       Sampler           = VK_NULL_HANDLE;
        VkBuffer        UploadBuffer      = VK_NULL_HANDLE;
        VkDeviceMemory  UploadBufferMemory= VK_NULL_HANDLE;
    };
}

#endif // HELLOIMGUI_HAS_VULKAN