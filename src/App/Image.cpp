// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Image.h"

#include "Application.h"

#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

#include "stb_image.h"

#include "Embed/DefaultImage.embed"

namespace GRAPE {
    namespace {
        std::uint32_t getVulkanMemoryType(VkMemoryPropertyFlags Properties, std::uint32_t TypeBits) {
            VkPhysicalDeviceMemoryProperties prop;
            vkGetPhysicalDeviceMemoryProperties(Application::vkPhysicalDevice(), &prop);
            for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
            {
                if ((prop.memoryTypes[i].propertyFlags & Properties) == Properties && TypeBits & 1 << i)
                    return i;
            }

            return 0xffffffff;
        }

        std::uint32_t bytesPerPixel(ImageFormat Format) {
            switch (Format)
            {
            case ImageFormat::None:    return 0;
            case ImageFormat::RGBA:    return 4;
            case ImageFormat::RGBA32F: return 16;
            default:                   return 0;
            }
        }

        VkFormat formatToVulkanFormat(ImageFormat Format) {
            switch (Format)
            {
            case ImageFormat::None:    return static_cast<VkFormat>(0);
            case ImageFormat::RGBA:    return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default:                   return static_cast<VkFormat>(0);
            }
        }

    }

    Image::Image(std::string_view Path) : m_Filepath(Path) {
        int width, height, channels;
        void* data;

        if (stbi_is_hdr(m_Filepath.c_str()))
        {
            data = stbi_loadf(m_Filepath.c_str(), &width, &height, &channels, 4);
            m_Format = ImageFormat::RGBA32F;
        }
        else
        {
            data = stbi_load(m_Filepath.c_str(), &width, &height, &channels, 4);
            m_Format = ImageFormat::RGBA;
        }

        if (!data)
        {
            setDefault();
            return;
        }

        m_Width = width;
        m_Height = height;

        allocateMemory();
        setData(data);

        stbi_image_free(data);
    }

    Image::Image(const void* Data, std::uint64_t Length) {
        std::uint32_t width, height;
        void* buffer = decode(Data, Length, width, height);

        m_Width = width;
        m_Height = height;
        m_Format = stbi_is_hdr_from_memory(static_cast<const stbi_uc*>(Data), static_cast<int>(Length)) ? ImageFormat::RGBA32F : ImageFormat::RGBA;

        allocateMemory();
        if (buffer)
        {
            setData(buffer);
            stbi_image_free(buffer);
        }
    }

    Image::Image(std::uint32_t Width, std::uint32_t Height, ImageFormat Format, const void* Data) : m_Width(Width), m_Height(Height), m_Format(Format) {
        allocateMemory();
        if (Data)
            setData(Data);
    }

    Image::~Image() {
        release();
    }

    void Image::allocateMemory() {
        VkDevice device = Application::vkDevice();

        VkResult err;

        VkFormat vulkanFormat = formatToVulkanFormat(m_Format);

        // Create the Image
        {
            VkImageCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = vulkanFormat;
            info.extent.width = m_Width;
            info.extent.height = m_Height;
            info.extent.depth = 1;
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            err = vkCreateImage(device, &info, nullptr, &m_Image);
            checkVkResult(err);
            VkMemoryRequirements req;
            vkGetImageMemoryRequirements(device, m_Image, &req);
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = req.size;
            allocInfo.memoryTypeIndex = getVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
            err = vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory);
            checkVkResult(err);
            err = vkBindImageMemory(device, m_Image, m_Memory, 0);
            checkVkResult(err);
        }

        // Create the Image View:
        {
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image = m_Image;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = vulkanFormat;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;
            err = vkCreateImageView(device, &info, nullptr, &m_ImageView);
            checkVkResult(err);
        }

        // Create sampler:
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            err = vkCreateSampler(device, &info, nullptr, &m_Sampler);
            checkVkResult(err);
        }

        // Create the Descriptor Set:
        m_DescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void Image::release() {
        const VkDevice device = Application::vkDevice();

        vkFreeMemory(device, m_StagingBufferMemory, nullptr);
        vkDestroyBuffer(device, m_StagingBuffer, nullptr);
        vkDestroySampler(device, m_Sampler, nullptr);
        vkDestroyImageView(device, m_ImageView, nullptr);
        vkDestroyImage(device, m_Image, nullptr);
        vkFreeMemory(device, m_Memory, nullptr);

        m_StagingBufferMemory = nullptr;
        m_StagingBuffer = nullptr;
        m_Sampler = nullptr;
        m_ImageView = nullptr;
        m_Image = nullptr;
        m_Memory = nullptr;
    }

    void Image::setData(const void* Data) {
        VkDevice device = Application::vkDevice();

        std::uint64_t uploadSize = static_cast<std::uint64_t>(m_Width) * static_cast<std::uint64_t>(m_Height) * static_cast<std::uint64_t>(bytesPerPixel(m_Format));

        VkResult err;

        if (!m_StagingBuffer)
            // Create the Upload Buffer
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = uploadSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            err = vkCreateBuffer(device, &bufferInfo, nullptr, &m_StagingBuffer);
            checkVkResult(err);
            VkMemoryRequirements req;
            vkGetBufferMemoryRequirements(device, m_StagingBuffer, &req);
            m_AlignedSize = req.size;
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = req.size;
            allocInfo.memoryTypeIndex = getVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
            err = vkAllocateMemory(device, &allocInfo, nullptr, &m_StagingBufferMemory);
            checkVkResult(err);
            err = vkBindBufferMemory(device, m_StagingBuffer, m_StagingBufferMemory, 0);
            checkVkResult(err);
        }

        // Upload to Buffer
        {
            char* map = nullptr;
            err = vkMapMemory(device, m_StagingBufferMemory, 0, m_AlignedSize, 0, reinterpret_cast<void**>(&map));
            checkVkResult(err);
            memcpy(map, Data, uploadSize);
            VkMappedMemoryRange range[1] = {};
            range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range[0].memory = m_StagingBufferMemory;
            range[0].size = m_AlignedSize;
            err = vkFlushMappedMemoryRanges(device, 1, range);
            checkVkResult(err);
            vkUnmapMemory(device, m_StagingBufferMemory);
        }

        // Create Command Buffer
        auto mainWindowData = Application::vkMainWindowData();
        VkCommandPool commandPool = mainWindowData->Frames[mainWindowData->FrameIndex].CommandPool;
        VkCommandBuffer commandBuffer;
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            err = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
            checkVkResult(err);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
            checkVkResult(err);
        }

        // Copy to Image
        {
            VkImageMemoryBarrier copyBarrier = {};
            copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copyBarrier.image = m_Image;
            copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyBarrier.subresourceRange.levelCount = 1;
            copyBarrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyBarrier);

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = m_Width;
            region.imageExtent.height = m_Height;
            region.imageExtent.depth = 1;
            vkCmdCopyBufferToImage(commandBuffer, m_StagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier useBarrier = {};
            useBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            useBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            useBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            useBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            useBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            useBarrier.image = m_Image;
            useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            useBarrier.subresourceRange.levelCount = 1;
            useBarrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &useBarrier);
        }

        // End command buffer
        {
            VkSubmitInfo endInfo = {};
            endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            endInfo.commandBufferCount = 1;
            endInfo.pCommandBuffers = &commandBuffer;
            err = vkEndCommandBuffer(commandBuffer);
            checkVkResult(err);
            err = vkQueueSubmit(Application::vkQueue(), 1, &endInfo, VK_NULL_HANDLE);
            checkVkResult(err);
            err = vkDeviceWaitIdle(device);
            checkVkResult(err);
        }
    }

    void Image::setDefault() {
        std::uint32_t width, height;
        void* buffer = decode(g_DefaultImage, sizeof g_DefaultImage, width, height);

        m_Width = width;
        m_Height = height;
        m_Format = stbi_is_hdr_from_memory(g_DefaultImage, sizeof g_DefaultImage) ? ImageFormat::RGBA32F : ImageFormat::RGBA;

        allocateMemory();
        if (buffer)
        {
            setData(buffer);
            stbi_image_free(buffer);
        }
    }

    void* Image::decode(const void* Buffer, std::uint64_t Length, std::uint32_t& OutWidth, std::uint32_t& OutHeight) {
        int width, height, channels;
        std::uint8_t* data = nullptr;

        data = stbi_load_from_memory(static_cast<const stbi_uc*>(Buffer), static_cast<int>(Length), &width, &height, &channels, 4);

        OutWidth = width;
        OutHeight = height;

        return data;
    }
}
