//
// Created by Sam on 2023-04-11.
//

#include "graphics/renderer.h"
#include "graphics/vulkan/vulkan_memory.h"
#include "graphics/vulkan/vulkan_instance.h"

void Renderer::createSurface() {
    if (glfwCreateWindowSurface(VulkanInstance::instance, this->window, nullptr, &this->surface) != VK_SUCCESS) {
        THROW("Failed to create window surface!");
    }
}

SwapchainSupportDetails Renderer::querySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    VkSurfaceFormatKHR out = availableFormats[0];

    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            out = availableFormat;
        }
    }

    VRB "Picked Swapchain Surface Format: " ENDL;
    VRB "\tFormat: " << out.format ENDL;
    VRB "\tColor Space: " << out.colorSpace ENDL;

    return out;
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    const std::vector<VkPresentModeKHR> presentModePreferences = {
            VK_PRESENT_MODE_FIFO_KHR,
            VK_PRESENT_MODE_IMMEDIATE_KHR,
            VK_PRESENT_MODE_MAILBOX_KHR
    };
    const std::vector<std::string> presentModeNames = {
            "V-Sync",
            "Uncapped",
            "Triple-Buffering"
    };
    uint32_t currentIndex = 0;

    for (const auto &availablePresentMode: availablePresentModes) {
        for (uint32_t i = 0; i < presentModePreferences.size(); ++i) {
            if (availablePresentMode == presentModePreferences[i] && i > currentIndex) {
                currentIndex = i;
            }
        }
    }

    INF "Picked Swapchain Present Mode: " << presentModeNames[currentIndex] ENDL;
    return presentModePreferences[currentIndex];
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    VkExtent2D out;
    // If the current extents are set to the maximum values,
    // the window manager is trying to tell us to set it manually.
    // Otherwise, return the current value.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        out = capabilities.currentExtent;
    } else {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        out = {
                static_cast<uint32_t>(w),
                static_cast<uint32_t>(h)
        };

        out.width = std::clamp(out.width,
                               capabilities.minImageExtent.width,
                               capabilities.maxImageExtent.width);
        out.height = std::clamp(out.height,
                                capabilities.minImageExtent.height,
                                capabilities.maxImageExtent.height);
    }

    this->framebufferWidth = out.width;
    this->framebufferHeight = out.height;

//    out.width /= 10;
//    out.height /= 10;
    INF "Swapchain extents set to: " << out.width << " * " << out.height ENDL;
    return out;
}

bool Renderer::recreateSwapchain() {
    VRB "Recreate Swapchain" ENDL;

    // May need to recreate render pass here if e.g. window moves to HDR monitor

    vkDeviceWaitIdle(VulkanDevices::logicalDevice);

    destroySwapchain();

    return createSwapchain();
}

bool Renderer::createSwapchain() {
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(VulkanDevices::physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

    if (extent.width < 1 || extent.height < 1) {
        VRB "Invalid swapchain extents. Retry later!" ENDL;
        this->needsNewSwapchain = true;
        return false;
    }

    // One more image than the minimum to avoid stalling if the driver is still working on the image
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }
    VRB "Creating the Swapchain with at least " << imageCount << " images!" ENDL;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // Can be 2 for 3D, etc.
    // TODO switch to VK_IMAGE_USAGE_TRANSFER_DST_BIT for post processing, instead of directly rendering to the SC
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueIndices[] = {this->queueFamilyIndices.graphicsFamily.value(),
                               this->queueFamilyIndices.presentFamily.value()};

    if (!this->queueFamilyIndices.isUnifiedGraphicsPresentQueue()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image is shared between queues -> no transfers!
        createInfo.queueFamilyIndexCount = 2; // Concurrent mode requires at least two indices
        createInfo.pQueueFamilyIndices = queueIndices; // Share image between these queues
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Image is owned by one queue at a time -> Perf+
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // Do not add any swapchain transforms beyond the default
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;

    // Do not blend with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;

    // Clip pixels if obscured by other window -> Perf+
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = nullptr; // Put previous swapchain here if overridden, e.g. if window size changed

    if (vkCreateSwapchainKHR(VulkanDevices::logicalDevice, &createInfo, nullptr, &this->swapchain) != VK_SUCCESS) {
        THROW("Failed to create swap chain!");
    }

    // imageCount only specified a minimum!
    vkGetSwapchainImagesKHR(VulkanDevices::logicalDevice, this->swapchain, &imageCount, nullptr);
    this->swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(VulkanDevices::logicalDevice, this->swapchain, &imageCount, this->swapchainImages.data());
    this->swapchainImageFormat = surfaceFormat.format;
    this->swapchainExtent = extent;

    createImageViews();
    createRenderPass();
    createFramebuffers();

    return true;
}

void Renderer::destroySwapchain() {
    for (auto &swapchainFramebuffer: this->swapchainFramebuffers) {
        vkDestroyFramebuffer(VulkanDevices::logicalDevice, swapchainFramebuffer, nullptr);
    }

    vkDestroyRenderPass(VulkanDevices::logicalDevice, this->renderPass, nullptr);

    for (auto &swapchainImageView: this->swapchainImageViews) {
        vkDestroyImageView(VulkanDevices::logicalDevice, swapchainImageView, nullptr);
    }

    vkDestroySwapchainKHR(VulkanDevices::logicalDevice, this->swapchain, nullptr);
}

void Renderer::createImageViews() {
    this->swapchainImageViews.resize(this->swapchainImages.size());

    for (size_t i = 0; i < this->swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = this->swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 1D - 3D or Cube maps
        createInfo.format = this->swapchainImageFormat;

        // Can swizzle all entities to be mapped to a single channel, or map to constants, etc.
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Color, no mipmapping, single layer
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1; // No 3D

        if (vkCreateImageView(VulkanDevices::logicalDevice, &createInfo, nullptr, &this->swapchainImageViews[i]) != VK_SUCCESS) {
            THROW("Failed to create image views!");
        }
    }
}

void Renderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

//    createImage(this->swapchainExtent.width,
//                this->swapchainExtent.height,
//                depthFormat,
//                VK_IMAGE_TILING_OPTIMAL,
//                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
//                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                this->depthImage,
//                this->depthImageMemory);
//    this->depthImageView = createImageView(depthImage, depthFormat);
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                       VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(VulkanDevices::physicalDevice, format, &props);

        if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
            (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            return format;
        }
    }

    THROW("Failed to find supported format!");
}

VkFormat Renderer::findDepthFormat() {
    return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL, // -> More efficient https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageTiling.html
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT // -> Can be used as depth/stencil attachment & input attachment
    );
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(VulkanDevices::logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        THROW("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(VulkanDevices::logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanMemory::findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(VulkanDevices::logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        THROW("Failed to allocate image memory!");
    }

    vkBindImageMemory(VulkanDevices::logicalDevice, image, imageMemory, 0);
}

void Renderer::createFramebuffers() {
    this->swapchainFramebuffers.resize(this->swapchainImageViews.size());

    for (size_t i = 0; i < this->swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
                this->swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = this->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = this->swapchainExtent.width;
        framebufferInfo.height = this->swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(VulkanDevices::logicalDevice, &framebufferInfo, nullptr, &this->swapchainFramebuffers[i]) !=
            VK_SUCCESS) {
            THROW("Failed to create framebuffer!");
        }
    }
}

bool Renderer::shouldRecreateSwapchain() {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    bool framebufferChanged = w != this->framebufferWidth || h != this->framebufferHeight;

    return this->needsNewSwapchain || framebufferChanged;
}