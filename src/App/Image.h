// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "vulkan/vulkan.h"

namespace GRAPE {
    enum class ImageFormat {
        None = 0,
        RGBA,
        RGBA32F,
    };

    class Image {
    public:
        explicit Image(std::string_view Path);
        Image(const void* Data, std::uint64_t Length);
        Image(std::uint32_t Width, std::uint32_t Height, ImageFormat Format, const void* Data = nullptr);
        ~Image();

        [[nodiscard]] VkDescriptorSet getDescriptorSet() const { return m_DescriptorSet; }
        [[nodiscard]] std::uint32_t width() const { return m_Width; }
        [[nodiscard]] std::uint32_t height() const { return m_Height; }

        void setData(const void* Data);
        void setDefault();

        static void* decode(const void* Buffer, std::uint64_t Length, std::uint32_t& OutWidth, std::uint32_t& OutHeight);
    private:
        std::uint32_t m_Width = 0, m_Height = 0;

        VkImage m_Image = nullptr;
        VkImageView m_ImageView = nullptr;
        VkDeviceMemory m_Memory = nullptr;
        VkSampler m_Sampler = nullptr;

        ImageFormat m_Format = ImageFormat::None;

        VkBuffer m_StagingBuffer = nullptr;
        VkDeviceMemory m_StagingBufferMemory = nullptr;

        std::size_t m_AlignedSize = 0;

        VkDescriptorSet m_DescriptorSet = nullptr;

        std::string m_Filepath;
    private:
        void allocateMemory();
        void release();
    };

}

