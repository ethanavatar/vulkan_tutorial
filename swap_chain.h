#pragma once
#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include <vulkan/vulkan.h>

VkResult createSwapChain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t graphicsFamily,
    uint32_t presentFamily,
    uint32_t width, uint32_t height,
    VkSwapchainKHR *swapChain,
    uint32_t *swapChainImageCount,
    VkImage **swapChainImages,
    VkFormat *swapChainImageFormat,
    VkExtent2D *swapChainExtent
);

#endif // SWAP_CHAIN_H
