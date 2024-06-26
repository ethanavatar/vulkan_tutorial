#pragma once
#ifndef SHADER_MODULES_H
#define SHADER_MODULES_H

#include <vulkan/vulkan.h>
//#include <shaderc/shaderc.h>

VkResult createShaderModule(
    VkDevice device,
    const char *shaderCode,
    size_t shaderCodeSize,
    VkShaderModule *shaderModule
);

#endif // SHADER_MODULES_H
