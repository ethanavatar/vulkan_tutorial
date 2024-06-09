#include <stdio.h>
#include <stdbool.h>

#include "defines.h"
#include "debug_messenger.h"
#include "extensions.h"

#include <vulkan/vulkan.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) {
    fprintf(stderr, "Validation layer: %s\n", pCallbackData->pMessage);
    UNUSED_INTENTIONAL(messageSeverity);
    UNUSED_INTENTIONAL(messageType);
    UNUSED_INTENTIONAL(pUserData);
    return VK_FALSE;
}

void fillDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *messengerInfo
) {
    fprintf(stderr, "fillDebugMessengerCreateInfo\n");
    messengerInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    messengerInfo->messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    messengerInfo->pfnUserCallback = debugCallback;
    messengerInfo->pUserData = NULL;
}

VkResult createDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT *debugMessenger
) {
    VkResult result = VK_SUCCESS;
    if (!ENABLE_VALIDATION_LAYERS) {
        fprintf(stderr, "setupDebugMessenger: Validation layers not enabled\n");
        return result;
    }

    fprintf(stderr, "Setting up debug messenger\n");

    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { 0 };
    fillDebugMessengerCreateInfo(&messengerInfo);

    PFN_vkCreateDebugUtilsMessengerEXT func = NULL;
    result = getExtensionFunction(instance, "vkCreateDebugUtilsMessengerEXT", (PFN_vkVoidFunction*) &func);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to load extension function: vkCreateDebugUtilsMessengerEXT");

    result = func(instance, &messengerInfo, NULL, debugMessenger);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create debug messenger");

    fprintf(stderr, "Debug messenger created\n");
    return result;
}

VkResult cleanupDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT *debugMessenger
) {
    VkResult result = VK_SUCCESS;
    if (!ENABLE_VALIDATION_LAYERS) {
        fprintf(stderr, "cleanupDebugMessenger: Validation layers not enabled\n");
        return result;
    }

    PFN_vkDestroyDebugUtilsMessengerEXT func = NULL;
    result = getExtensionFunction(instance, "vkDestroyDebugUtilsMessengerEXT", (PFN_vkVoidFunction*) &func);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to load extension function: vkDestroyDebugUtilsMessengerEXT");

    func(instance, *debugMessenger, NULL);

    fprintf(stderr, "Debug messenger destroyed\n");
    return result;
}
