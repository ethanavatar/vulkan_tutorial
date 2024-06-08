#pragma once
#ifndef DEBUG_MESSENGER_H
#define DEBUG_MESSENGER_H

#include <vulkan/vulkan.h>

void fillDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *messengerInfo
);

VkResult createDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT *debugMessenger
);

VkResult cleanupDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT *debugMessenger
);

#endif // DEBUG_MESSENGER_H
