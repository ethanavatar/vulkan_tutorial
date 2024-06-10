#include <vulkan/vulkan.h>
//#include <shaderc/shaderc.h>

VkResult createShaderModule(
    VkDevice device,
    const char *shaderCode,
    size_t shaderCodeSize,
    VkShaderModule *shaderModule
) {
    VkShaderModuleCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCodeSize;
    createInfo.pCode = (uint32_t*)shaderCode;

    return vkCreateShaderModule(device, &createInfo, NULL, shaderModule);
}

