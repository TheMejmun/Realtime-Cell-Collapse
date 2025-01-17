//
// Created by Sam on 2023-04-11.
//

#include "graphics/renderer.h"

VkShaderModule Renderer::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // Cast the pointer. Vectors already handle proper memory alignment.
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(VulkanDevices::logical, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        THROW("Failed to create shader module!");
    }

    return shaderModule;
}
