#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

#include <assert.h>
#include <stdlib.h>

#include "defines.h"

// TODO: internal headers
static VkSurfaceFormatKHR getSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
static VkPresentModeKHR getPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
static VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR capabilities, uint32_t width, uint32_t height);

VkResult createSwapChain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t graphicsFamily,
    uint32_t presentFamily,
    uint32_t windowWidth, uint32_t windowHeight,
    VkSwapchainKHR *swapChain,
    uint32_t *swapChainImageCount,
    VkImage **swapChainImages,
    VkFormat *swapChainImageFormat,
    VkExtent2D *swapChainExtent
) {
    VkResult result;

    VkSurfaceCapabilitiesKHR capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get surface capabilities");

    VkSurfaceFormatKHR surfaceFormat = getSurfaceFormat(physicalDevice, surface);
    VkPresentModeKHR presentMode = getPresentMode(physicalDevice, surface);
    VkExtent2D extent = chooseExtent(capabilities, windowWidth, windowHeight);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };

    if (graphicsFamily != presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(device, &createInfo, NULL, swapChain);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create swap chain");

    vkGetSwapchainImagesKHR(device, *swapChain, &imageCount, NULL);
    VkImage *images = malloc(imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, *swapChain, &imageCount, images);

    *swapChainImages = images;
    *swapChainImageCount = imageCount;
    *swapChainImageFormat = surfaceFormat.format;
    *swapChainExtent = extent;

    return VK_SUCCESS;
}

static VkSurfaceFormatKHR getSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, NULL);
    assert(count > 0);

    VkSurfaceFormatKHR *formats = alloca(count * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats);

    VkSurfaceFormatKHR format = formats[0];
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format != VK_FORMAT_B8G8R8A8_UNORM) continue;
        if (formats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) continue;
        format = formats[i];
    }

    return format;
}

static VkPresentModeKHR getPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, NULL);
    assert(count > 0);

    VkPresentModeKHR *modes = alloca(count * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, modes);

    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < count; i++) {
        if (modes[i] != VK_PRESENT_MODE_MAILBOX_KHR) continue;
        mode = modes[i];
    }

    return mode;
}

// Usually you'd use fmax/fmin for this but
// MSVC doesn't allow implicit conversions from uint32_t into fmax/fmin
// so it has max/min which are not standard
// TODO: These are unsafe because they can cause double evaluation
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    return (VkExtent2D) {
        .width = MAX(
            capabilities.minImageExtent.width,
            MIN(capabilities.maxImageExtent.width, width)
        ),
        .height = MAX(
            capabilities.minImageExtent.height,
            MIN(capabilities.maxImageExtent.height, height)
        )
    };
}
