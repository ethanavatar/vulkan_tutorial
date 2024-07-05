#pragma once
#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include <vulkan/vulkan.h>

struct SwapChain {
    VkSwapchainKHR vkSwapChain;
    uint32_t imageCount;
    VkImage *images;             // has `imageCount` elements
    VkImageView *imageViews;     // has `imageCount` elements
    VkFramebuffer *framebuffers; // has `imageCount` elements
    VkFormat imageFormat;
    VkExtent2D extent;
};

VkResult createSwapChain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t graphicsFamily,
    uint32_t presentFamily,
    uint32_t width, uint32_t height,
    struct SwapChain *swapChain
);

void cleanupSwapChain(
    VkDevice device,
    struct SwapChain swapChain
);

#endif // SWAP_CHAIN_H
