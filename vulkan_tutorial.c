#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#ifdef __cplusplus
#include <vulkan/vk_enum_string_helper.h>
#else
#define VK_ENUM_CSTRING_HELPER_IMPLEMENTATION
#include "vk_enum_cstring_helper.h"
#endif

#define FILE_IO_IMPLEMENTATION
#include "file_io.h"

#include <cglm/cglm.h>

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define STRINGIFY(x) #x
#define UNUSED_INTENTIONAL(x) { (void) (x); }
#define UNUSED(x) {\
    fprintf(stderr, "UNUSED VARIABLE: %s\n", STRINGIFY(x));\
    (void) (x); }

#define TODO(comment) {\
    fprintf(stderr, "TODO (%s:%d): %s\n", __FILE__, __LINE__, comment); }

#define RETURN_IF_NOT_VK_SUCCESS(R, MSG) do {\
    if (R != VK_SUCCESS) {\
        fprintf(stderr, "Error: %s\n", MSG);\
        return R;\
    }\
} while(0)

const uint32_t initialWindowWidth = 800;
const uint32_t initialWindowHeight = 600;

#define REQUESTED_VALIDATION_LAYERS 1
const char* validationLayers[REQUESTED_VALIDATION_LAYERS] = {
    "VK_LAYER_KHRONOS_validation"
};

#define REQUESTED_DEVICE_EXTENSIONS 1
static const char* deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
#    define ENABLE_VALIDATION_LAYERS false
#else
#    define ENABLE_VALIDATION_LAYERS true
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) {
    fprintf(stderr, "Validation layer: %s\n", pCallbackData->pMessage);
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

bool checkValidationLayers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    fprintf(stderr, "checkValidationLayers: Layer count: %d\n", layerCount);

    if (layerCount == 0) return false;

    // Smh, MSVC, I can't use a VLA here
    VkLayerProperties *availableLayers;
    availableLayers = malloc(layerCount * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (uint32_t i = 0; i < REQUESTED_VALIDATION_LAYERS; i++) {
        bool layerFound = false;

        for (uint32_t j = 0; j < layerCount; j++) {
            if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    free(availableLayers);
    return true;
}

VkResult createInstance(
    VkInstance *Instance
) {
    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayers()) {
        fprintf(stderr, "Validation layers requested, but not available\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    fprintf(stderr, "Creating Vulkan instance\n");

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };

    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { 0 };
    if (ENABLE_VALIDATION_LAYERS) {
        instanceCreateInfo.enabledLayerCount = REQUESTED_VALIDATION_LAYERS;
        instanceCreateInfo.ppEnabledLayerNames = validationLayers;

        fillDebugMessengerCreateInfo(&messengerInfo);
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &messengerInfo;
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = NULL;
    }

    // Required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    fprintf(stderr, "GLFW extensions: %d\n", glfwExtensionCount);

    // Optional extensions
    uint32_t extensionCount = glfwExtensionCount;

    const char* debugExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    if (ENABLE_VALIDATION_LAYERS) extensionCount++;

    const char** extensions = malloc(extensionCount * sizeof(const char*));
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        extensions[i] = glfwExtensions[i];
    }

    if (ENABLE_VALIDATION_LAYERS) {
        extensions[glfwExtensionCount] = debugExtension;
    }

    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensions;

    return vkCreateInstance(&instanceCreateInfo, NULL, Instance);
}

VkResult getExtensionFunction(
    VkInstance instance,
    const char *const name,
    PFN_vkVoidFunction *function
) {
    *function = vkGetInstanceProcAddr(instance, name);
    if (*function == NULL) {
        fprintf(stderr, "Failed to load extension function: %s\n", name);
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return VK_SUCCESS;
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

VkResult getQueueFamilies(
    VkPhysicalDevice device,
    VkQueueFlags flags,
    uint32_t *graphicsFamily
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies;
    queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        queueFamilies
    );

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & flags) {
            *graphicsFamily = i;
            free(queueFamilies);
            return VK_SUCCESS;
        }
    }

    free(queueFamilies);
    return VK_ERROR_INITIALIZATION_FAILED;
}

VkResult getPresentQueueFamilies(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    uint32_t *presentFamily
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies;
    queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        queueFamilies
    );

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (queueFamilies[i].queueCount > 0 && presentSupport) {
            *presentFamily = i;
            free(queueFamilies);
            return VK_SUCCESS;
        }
    }

    free(queueFamilies);
    return VK_ERROR_INITIALIZATION_FAILED;
}

bool deviceHasQueueFamilyFlags(
    VkPhysicalDevice device,
    VkQueueFlags flags
) {
    uint32_t _graphicsFamily;
    VkResult result = getQueueFamilies(device, flags, &_graphicsFamily);
    UNUSED_INTENTIONAL(_graphicsFamily);
    return result == VK_SUCCESS;
}

int scoreDeviceCapabilities(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    if (!deviceHasQueueFamilyFlags(device, VK_QUEUE_GRAPHICS_BIT)) {
        return 0;
    }

    // Check for required device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions;
    availableExtensions = malloc(extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    for (uint32_t i = 0; i < REQUESTED_DEVICE_EXTENSIONS; i++) {
        bool extensionFound = false;
        for (uint32_t j = 0; j < extensionCount; j++) {
            if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound) {
            free(availableExtensions);
            return 0;
        }
    }

    // Check for swap chain support
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
    if (result != VK_SUCCESS) {
        free(availableExtensions);
        return 0;
    }

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    if (formatCount == 0) {
        free(availableExtensions);
        return 0;
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
    if (presentModeCount == 0) {
        free(availableExtensions);
        return 0;
    }

    free(availableExtensions);
    return score;
}

VkResult getPhysicalDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    VkPhysicalDevice *physicalDevice
) {
    *physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        fprintf(stderr, "No physical devices found\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // Find the device with the highest score
    int bestScore = 0;
    int bestIndex = -1;
    for (uint32_t i = 0; i < deviceCount; i++) {
        int score = scoreDeviceCapabilities(devices[i], surface);
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestIndex == -1) {
        fprintf(stderr, "No suitable physical device found\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    *physicalDevice = devices[bestIndex];
    free(devices);

    return VK_SUCCESS;
}

VkResult createLogicalDevice(
    VkPhysicalDevice physicalDevice,
    uint32_t graphicsFamily,
    VkDevice *device
) {
    VkResult result;

    VkDeviceQueueCreateInfo queueCreateInfo = { 0 };
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = { 0 };

    VkDeviceCreateInfo deviceCreateInfo = { 0 };
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (ENABLE_VALIDATION_LAYERS) {
        deviceCreateInfo.enabledLayerCount = REQUESTED_VALIDATION_LAYERS;
        deviceCreateInfo.ppEnabledLayerNames = validationLayers;
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    // Enable device extensions
    deviceCreateInfo.enabledExtensionCount = REQUESTED_DEVICE_EXTENSIONS;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    return vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, device);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    VkSurfaceFormatKHR *availableFormats,
    uint32_t formatCount
) {
    for (uint32_t i = 0; i < formatCount; i++) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    VkPresentModeKHR *availableModes,
    uint32_t modeCount
) {
    for (uint32_t i = 0; i < modeCount; i++) {
        if (availableModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availableModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(
    VkSurfaceCapabilitiesKHR capabilities
) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = { initialWindowWidth, initialWindowHeight };

    actualExtent.width = fmax(
        capabilities.minImageExtent.width,
        fmin(capabilities.maxImageExtent.width, actualExtent.width)
    );

    actualExtent.height = fmax(
        capabilities.minImageExtent.height,
        fmin(capabilities.maxImageExtent.height, actualExtent.height)
    );

    return actualExtent;
}

VkResult createSwapChain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t graphicsFamily,
    uint32_t presentFamily,
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

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    if (formatCount == 0) return VK_ERROR_INITIALIZATION_FAILED;

    VkSurfaceFormatKHR *availableFormats;
    availableFormats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, availableFormats);

    VkFormat imageFormat = chooseSwapSurfaceFormat(availableFormats, formatCount).format;
    VkColorSpaceKHR colorSpace = chooseSwapSurfaceFormat(availableFormats, formatCount).colorSpace;

    free(availableFormats);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    if (presentModeCount == 0) return VK_ERROR_INITIALIZATION_FAILED;

    VkPresentModeKHR *availablePresentModes;
    availablePresentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, availablePresentModes);

    VkPresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes, presentModeCount);

    free(availablePresentModes);

    VkExtent2D extent = chooseSwapExtent(capabilities);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = imageFormat;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
    *swapChainImageFormat = imageFormat;
    *swapChainExtent = extent;

    return VK_SUCCESS;
}

VkResult createImageViews(
    VkDevice device,
    VkImage *swapChainImages,
    uint32_t imageCount,
    VkFormat imageFormat,
    VkImageView *swapChainImageViews
) {
    VkResult result = VK_SUCCESS;
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo createInfo = { 0 };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]);
        if (result != VK_SUCCESS) break;
    }

    return result;
}

VkResult createRenderPass(
    VkDevice device,
    VkFormat imageFormat,
    VkRenderPass *renderPass
) {
    VkAttachmentDescription colorAttachment = { 0 };
    colorAttachment.format = imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = { 0 };
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    return vkCreateRenderPass(device, &renderPassInfo, NULL, renderPass);
}


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

VkResult createGraphicsPipeline(
    VkDevice device,
    VkExtent2D swapChainExtent,
    VkRenderPass renderPass,
    VkPipelineLayout *pipelineLayout,
    VkPipeline *graphicsPipeline
) {
    VkResult result = VK_SUCCESS;
    /*
    VkResult result = createShaderModule(
        device,
        "shaders/vert.glsl",
        SHADER_TYPE_VERTEX,
        &vertShaderModule
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create vertex shader module");

    result = createShaderModule(
        device,
        "shaders/frag.glsl",
        SHADER_TYPE_FRAGMENT,
        &fragShaderModule
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create fragment shader module");
    */

    size_t size;
    const char *vertShaderCode = read_file_bytes("shaders/vert.spv", &size);
    fprintf(stderr, "vertex shader size: %zu\n", size);

    VkShaderModule vertShaderModule;
    result = createShaderModule(device, vertShaderCode, size, &vertShaderModule);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create vertex shader module");

    const char *fragShaderCode = read_file_bytes("shaders/frag.spv", &size);
    fprintf(stderr, "fragment shader size: %zu\n", size);

    VkShaderModule fragShaderModule;
    result = createShaderModule(device, fragShaderCode, size, &fragShaderModule);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create fragment shader module");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = { 0 };
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = { 0 };
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D) { 0, 0 };
    scissor.extent = swapChainExtent;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo viewportState = { 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create pipeline layout");

    VkGraphicsPipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    result = vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        graphicsPipeline
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create graphics pipeline");

    return result;
}

#define QUEUE_FAMILIES_COUNT 2
static struct RenderState {
    VkInstance instance;

    GLFWwindow* window;
    VkSurfaceKHR windowSurface;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue deviceQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    uint32_t swapChainImageCount;
    VkImage *swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkImageView *swapChainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
} state;

VkResult renderInit() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(1);
    }
    fprintf(stderr, "GLFW initialized: %s\n", glfwGetVersionString());
    fprintf(stderr, "Vulkan supported: %s\n", glfwVulkanSupported() ? "yes" : "no");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(
        initialWindowWidth,
        initialWindowHeight,
        "Vulkan Triangle",
        NULL,
        NULL
    );
    state.window = window;

    fprintf(stderr, "Initializing Vulkan\n");
    VkInstance instance;
    VkResult result = createInstance(&instance);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create Vulkan instance");
    state.instance = instance;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create window surface");
    state.windowSurface = surface;

    TODO("Some sort of optional type for debugMessenger")
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (ENABLE_VALIDATION_LAYERS) {
        result = createDebugMessenger(instance, &debugMessenger);
        RETURN_IF_NOT_VK_SUCCESS(result, "Failed to initialize debug messenger");
    }
    state.debugMessenger = debugMessenger;

    VkPhysicalDevice physicalDevice;
    result = getPhysicalDevice(instance, surface, &physicalDevice);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get physical device");
    state.physicalDevice = physicalDevice;

    uint32_t graphicsFamily;
    VkQueueFlags flags = VK_QUEUE_GRAPHICS_BIT;
    result = getQueueFamilies(physicalDevice, flags, &graphicsFamily);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get graphics queue family");

    VkDevice device;
    result = createLogicalDevice(physicalDevice, graphicsFamily, &device);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create logical device");
    state.device = device;

    VkQueue deviceQueue;
    vkGetDeviceQueue(device, graphicsFamily, 0, &deviceQueue);
    state.deviceQueue = deviceQueue;

    uint32_t presentFamily;
    result = getPresentQueueFamilies(physicalDevice, surface, &presentFamily);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get present queue family");

    VkQueue presentQueue;
    vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
    state.presentQueue = presentQueue;

    VkSwapchainKHR swapChain;
    uint32_t imageCount;
    VkImage *swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    TODO("Fix this abomination of a function signature");
    result = createSwapChain(
        physicalDevice,
        device,
        surface,
        graphicsFamily,
        presentFamily,
        &swapChain,
        &imageCount,
        &swapChainImages,
        &swapChainImageFormat,
        &swapChainExtent
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create swap chain");
    state.swapChain = swapChain;
    state.swapChainImageCount = imageCount;
    state.swapChainImages = swapChainImages;
    state.swapChainImageFormat = swapChainImageFormat;
    state.swapChainExtent = swapChainExtent;

    VkImageView *swapChainImageViews;
    swapChainImageViews = malloc(imageCount * sizeof(VkImageView));
    result = createImageViews(device, swapChainImages, imageCount, swapChainImageFormat, swapChainImageViews);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create image views");
    state.swapChainImageViews = swapChainImageViews;

    VkRenderPass renderPass;
    result = createRenderPass(device, swapChainImageFormat, &renderPass);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create render pass");
    state.renderPass = renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    result = createGraphicsPipeline(device, swapChainExtent, renderPass, &pipelineLayout, &graphicsPipeline);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create graphics pipeline");
    state.pipelineLayout = pipelineLayout;
    state.graphicsPipeline = graphicsPipeline;

    fprintf(stderr, "Vulkan context initialized successfully\n");
    return VK_SUCCESS;
}

void vulkanCleanup() {
    fprintf(stderr, "Cleaning up Vulkan\n");
    if (ENABLE_VALIDATION_LAYERS) {
        VkResult result = cleanupDebugMessenger(
            state.instance,
            &state.debugMessenger
        );
        UNUSED_INTENTIONAL(result);
    }

    for (uint32_t i = 0; i < state.swapChainImageCount; i++) {
        vkDestroyImageView(state.device, state.swapChainImageViews[i], NULL);
    }
    free(state.swapChainImageViews);

    vkDestroyPipeline(state.device, state.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(state.device, state.pipelineLayout, NULL);
    vkDestroyRenderPass(state.device, state.renderPass, NULL);

    vkDestroySwapchainKHR(state.device, state.swapChain, NULL);
    vkDestroyDevice(state.device, NULL);
    vkDestroySurfaceKHR(state.instance, state.windowSurface, NULL);
    vkDestroyInstance(state.instance, NULL);

    glfwDestroyWindow(state.window);
    glfwTerminate();
}

int main(void) {
    VkResult result = renderInit();
    if (result != VK_SUCCESS) {
        char *result_str = string_VkResult(result);
        fprintf(stderr, "Failed to initialize Vulkan: %s\n", result_str);
        exit(1);
    }

    while(!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();
    }

    vulkanCleanup();
    exit(0);
    return 0;
}
