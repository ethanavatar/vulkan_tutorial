#pragma once
#ifndef VULKAN_EXTENSIONS_H
#define VULKAN_EXTENSIONS_H

#include <vulkan/vulkan.h>

VkResult getExtensionFunction(
    VkInstance instance,
    const char *const name,
    PFN_vkVoidFunction *function
);

#endif // VULKAN_EXTENSIONS_H
