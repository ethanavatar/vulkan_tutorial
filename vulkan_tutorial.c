#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "defines.h"
#include "debug_messenger.h"
#include "extensions.h"
#include "shader_modules.h"
#include "swap_chain.h"

#ifdef __cplusplus
#include <vulkan/vk_enum_string_helper.h>
#else
#define VK_ENUM_CSTRING_HELPER_IMPLEMENTATION
#include "vk_enum_cstring_helper.h"
#endif

#define FILE_IO_IMPLEMENTATION
#include "file_io.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define REQUESTED_VALIDATION_LAYERS 1
static const char* validationLayers[REQUESTED_VALIDATION_LAYERS] = {
    "VK_LAYER_KHRONOS_validation"
};

#define REQUESTED_DEVICE_EXTENSIONS 1
static const char* deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const uint32_t initialWindowWidth = 800;
const uint32_t initialWindowHeight = 600;
const uint32_t maxFramesInFlight = 2;

bool checkValidationLayers(void) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    fprintf(stderr, "checkValidationLayers: Layer count: %d\n", layerCount);

    if (layerCount == 0) return false;

    // Smh, MSVC, I can't use a VLA here
    VkLayerProperties *availableLayers = alloca(layerCount * sizeof(VkLayerProperties));
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

    return true;
}

VkResult createVulkanInstance(VkInstance *outInstance) {
    VkResult result;
    // This tricks MSVC into not thinking the conditional is constant
    bool enableValidationLayers = ENABLE_VALIDATION_LAYERS;
    if (enableValidationLayers && !checkValidationLayers()) {
        fprintf(stderr, "Validation layers requested, but not available\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    fprintf(stderr, "Creating Vulkan instance\n");

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
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

    const char** extensions = alloca(extensionCount * sizeof(const char*));
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        extensions[i] = glfwExtensions[i];
    }

    if (ENABLE_VALIDATION_LAYERS) {
        extensions[glfwExtensionCount] = debugExtension;
    }

    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensions;

    result = vkCreateInstance(&instanceCreateInfo, NULL, outInstance);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create instance");
    return VK_SUCCESS;
}

VkResult getQueueFamilies(
    VkPhysicalDevice device,
    VkQueueFlags flags,
    uint32_t *graphicsFamily
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies;
    queueFamilies = alloca(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        queueFamilies
    );

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & flags) {
            *graphicsFamily = i;
            return VK_SUCCESS;
        }
    }

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
    queueFamilies = alloca(queueFamilyCount * sizeof(VkQueueFamilyProperties));
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
            return VK_SUCCESS;
        }
    }

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

    TODO("robustBufferAccess shouldnt be required");
    if (!deviceFeatures.robustBufferAccess) {
        return 0;
    }

    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    if (!deviceHasQueueFamilyFlags(device, VK_QUEUE_GRAPHICS_BIT)) {
        return 0;
    }

    // Check for required device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions = alloca(extensionCount * sizeof(VkExtensionProperties));
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
            return 0;
        }
    }

    // Check for swap chain support
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
    if (result != VK_SUCCESS) {
        return 0;
    }

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    if (formatCount == 0) {
        return 0;
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
    if (presentModeCount == 0) {
        return 0;
    }

    return score;
}

VkResult getPhysicalDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    VkPhysicalDevice *outPhysicalDevice
) {
    VkResult result;
    *outPhysicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to enumerate physical devices");

    if (deviceCount == 0) {
        fprintf(stderr, "No physical devices found\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkPhysicalDevice *devices = alloca(deviceCount * sizeof(VkPhysicalDevice));
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to enumerate physical devices");

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

    *outPhysicalDevice = devices[bestIndex];

    return VK_SUCCESS;
}

VkResult createLogicalDevice(
    VkPhysicalDevice physicalDevice,
    uint32_t graphicsFamily,
    VkDevice *outDevice
) {
    VkResult result;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = (float[]) { 1.0f }
    };

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = REQUESTED_DEVICE_EXTENSIONS,
        .ppEnabledExtensionNames = deviceExtensions
    };

    if (ENABLE_VALIDATION_LAYERS) {
        deviceCreateInfo.enabledLayerCount = REQUESTED_VALIDATION_LAYERS;
        deviceCreateInfo.ppEnabledLayerNames = validationLayers;
    }

    result = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, outDevice);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create logical device");
    return VK_SUCCESS;
}

VkResult createRenderPass(
    VkDevice device,
    VkFormat imageFormat,
    VkRenderPass *outRenderPass
) {
    VkAttachmentDescription colorAttachment = {
        .format = imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, outRenderPass);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create render pass");
    return result;
}

#include <cglm/cglm.h>

struct Vertex {
    vec2 pos;
    vec3 color;
};

const struct Vertex vertices[] = {
    { {  0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    { {  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } }
};

static VkVertexInputBindingDescription getBindingDescription(void) {
    VkVertexInputBindingDescription bindingDescription = {
        .binding = 0,
        .stride = sizeof(struct Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    return bindingDescription;
}

struct AttributeDescriptions {
    VkVertexInputAttributeDescription pos;
    VkVertexInputAttributeDescription color;
};

static struct AttributeDescriptions getAttributeDescriptions(void) {
    struct AttributeDescriptions attributeDescriptions = { 0 };

    attributeDescriptions.pos = (VkVertexInputAttributeDescription) {
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(struct Vertex, pos)
    };

    attributeDescriptions.color = (VkVertexInputAttributeDescription) {
        .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(struct Vertex, color)
    };

    return attributeDescriptions;
}

static const VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

VkResult createGraphicsPipeline(
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout *pipelineLayout,
    VkPipeline *outGraphicsPipeline
) {
    VkResult result;

    size_t size;
    const char *vertShaderCode = read_entire_file("shaders/vert.spv", &size);
    VkShaderModule vertShaderModule;
    result = createShaderModule(device, vertShaderCode, size, &vertShaderModule);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create vertex shader module");
    free((void *) vertShaderCode);

    const char *fragShaderCode = read_entire_file("shaders/frag.spv", &size);
    VkShaderModule fragShaderModule;
    result = createShaderModule(device, fragShaderCode, size, &fragShaderModule);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create fragment shader module");
    free((void *) fragShaderCode);

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

    VkVertexInputBindingDescription bindingDescription = getBindingDescription();
    struct AttributeDescriptions attributeDescriptions = getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .vertexAttributeDescriptionCount = 2,
        .pVertexBindingDescriptions = &bindingDescription,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
            attributeDescriptions.pos,
            attributeDescriptions.color
        }
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

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
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

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
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDynamicState = &dynamicState;

    result = vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        outGraphicsPipeline
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create graphics pipeline");

    vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);

    return result;
}

VkResult createFramebuffers(
    VkDevice device,
    VkRenderPass renderPass,
    VkExtent2D swapChainExtent,
    VkImageView *swapChainImageViews,
    uint32_t imageCount,
    VkFramebuffer *framebuffers
) {
    VkResult result = VK_SUCCESS;

    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageView attachments[] = { swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = { 0 };
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]); 
        RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create framebuffer");
    }

    return result;
}

VkResult createCommandPool(
    VkDevice device,
    uint32_t graphicsFamily,
    VkCommandPool *commandPool
) {
    VkCommandPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily;

    return vkCreateCommandPool(device, &poolInfo, NULL, commandPool);
}

VkResult createCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandBuffer *commandBuffers[],
    uint32_t commandBufferCount
) {
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    return vkAllocateCommandBuffers(device, &allocInfo, *commandBuffers);
}

VkResult recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer vertexBuffer,
    uint32_t imageIndex,
    VkRenderPass renderPass,
    VkFramebuffer *swapChainFramebuffers,
    VkExtent2D swapChainExtent,
    VkPipeline graphicsPipeline
) {
    VkResult result = VK_SUCCESS;

    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = NULL;

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    PANIC_IF_NOT_VK_SUCCESS(result, "Failed to begin recording command buffer");

    VkRenderPassBeginInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = (VkOffset2D) { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) swapChainExtent.width,
            .height = (float) swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = swapChainExtent
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);
    PANIC_IF_NOT_VK_SUCCESS(result, "Failed to record command buffer");

    return result;
}

VkResult createSyncObjects(
    VkDevice device,
    VkSemaphore *imageAvailableSemaphores[],
    VkSemaphore *renderFinishedSemaphores[],
    VkFence *inFlightFences[]
) {
    VkResult result = VK_SUCCESS;

    VkSemaphoreCreateInfo semaphoreInfo = { 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &(*imageAvailableSemaphores)[i]);
        if (result != VK_SUCCESS) return result;

        result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &(*renderFinishedSemaphores)[i]);
        if (result != VK_SUCCESS) return result;

        result = vkCreateFence(device, &fenceInfo, NULL, &(*inFlightFences)[i]);
        if (result != VK_SUCCESS) return result;
    }

    return result;
}

uint32_t findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (
            (typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties
        ) {
            return i;
        }
    }

    return -1;
}

VkResult createVertexBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkBuffer *outVertexBuffer,
    VkDeviceMemory *outVertexBufferMemory
) {
    VkResult result;
    fprintf(stderr, "sizeof(vertices) = %lu\n", sizeof(vertices));
    fprintf(stderr, "sizeof(vertices[0]) = %lu\n", sizeof(vertices[0]));
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    VkBuffer vertexBuffer;
    result = vkCreateBuffer(device, &bufferInfo, NULL, &vertexBuffer);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create vertex buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(
            physicalDevice,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    };
    
    VkDeviceMemory vertexBufferMemory;
    result = vkAllocateMemory(device, &allocInfo, NULL, &vertexBufferMemory);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to allocate vertex buffer memory");

    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    void *data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(device, vertexBufferMemory);

    *outVertexBuffer = vertexBuffer;
    *outVertexBufferMemory = vertexBufferMemory;

    return VK_SUCCESS;
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

    uint32_t graphicsFamily;
    uint32_t presentFamily;

    bool framebufferResized;
    struct SwapChain swapChain;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkCommandPool commandPool;
    VkCommandBuffer *commandBuffers;

    VkSemaphore *imageAvailableSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;

    uint32_t currentFrame;
} state;

VkResult recreateSwapChain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t graphicsFamily,
    uint32_t presentFamily,
    VkRenderPass renderPass,
    uint32_t fenceCount,
    VkFence *inFlightFences,
    struct SwapChain *activeSwapChain
) {
    VkResult result;

    int width = 0, height = 0;
    glfwGetFramebufferSize(state.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(state.window, &width, &height);
        glfwWaitEvents();
    }

    fprintf(stderr, "New dimensions: %dx%d\n", width, height);

    result = vkDeviceWaitIdle(device);
    PANIC_IF_NOT_VK_SUCCESS(result, "Failed to wait for device to be idle");

    result = vkWaitForFences(device, fenceCount, inFlightFences, VK_TRUE, UINT64_MAX);
    PANIC_IF_NOT_VK_SUCCESS(result, "Failed to wait for frames in-flight to finish");

    cleanupSwapChain(device, activeSwapChain);
    result = createSwapChain(
        physicalDevice,
        device,
        surface,
        graphicsFamily,
        presentFamily,
        width,
        height,
        activeSwapChain
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to recreate swap chain");

    result = createFramebuffers(
        device,
        renderPass,
        activeSwapChain->extent,
        activeSwapChain->imageViews,
        activeSwapChain->imageCount,
        activeSwapChain->framebuffers
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to recreate framebuffers");

    return VK_SUCCESS;
}

VkResult renderInit(void) {
    VkResult result;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(1);
    }
    fprintf(stderr, "GLFW initialized: %s\n", glfwGetVersionString());
    fprintf(stderr, "Vulkan supported: %s\n", glfwVulkanSupported() ? "yes" : "no");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    FIXME("Crashes during window resizing. Seems to be supressed by using robustBufferAccess");

    state.window = glfwCreateWindow(
        initialWindowWidth,
        initialWindowHeight,
        "Vulkan Triangle",
        NULL,
        NULL
    );

    fprintf(stderr, "Initializing Vulkan\n");
    result = createVulkanInstance(&state.instance);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create Vulkan instance");

    result = glfwCreateWindowSurface(state.instance, state.window, NULL, &state.windowSurface);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create window surface");

    state.debugMessenger = VK_NULL_HANDLE;
    if (ENABLE_VALIDATION_LAYERS) {
        result = createDebugMessenger(state.instance, &state.debugMessenger);
        RETURN_IF_NOT_VK_SUCCESS(result, "Failed to initialize debug messenger");
    }

    result = getPhysicalDevice(state.instance, state.windowSurface, &state.physicalDevice);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get physical device");

    uint32_t graphicsFamily;
    VkQueueFlags flags = VK_QUEUE_GRAPHICS_BIT;
    result = getQueueFamilies(state.physicalDevice, flags, &graphicsFamily);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get graphics queue family");
    state.graphicsFamily = graphicsFamily;

    VkDevice device;
    result = createLogicalDevice(state.physicalDevice, graphicsFamily, &device);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create logical device");
    state.device = device;

    VkQueue deviceQueue;
    vkGetDeviceQueue(device, graphicsFamily, 0, &deviceQueue);
    state.deviceQueue = deviceQueue;

    uint32_t presentFamily;
    result = getPresentQueueFamilies(state.physicalDevice, state.windowSurface, &presentFamily);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to get present queue family");
    state.presentFamily = presentFamily;

    VkQueue presentQueue;
    vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
    state.presentQueue = presentQueue;

    struct SwapChain swapChain;
    result = createSwapChain(
        state.physicalDevice,
        device,
        state.windowSurface,
        graphicsFamily,
        presentFamily,
        initialWindowWidth, initialWindowHeight,
        &swapChain
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create swap chain");
    state.swapChain = swapChain;

    VkRenderPass renderPass;
    result = createRenderPass(device, swapChain.imageFormat, &renderPass);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create render pass");
    state.renderPass = renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    result = createGraphicsPipeline(
        device,
        renderPass,
        &pipelineLayout,
        &graphicsPipeline
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create graphics pipeline");
    state.pipelineLayout = pipelineLayout;
    state.graphicsPipeline = graphicsPipeline;

    VkFramebuffer *framebuffers = malloc(swapChain.imageCount * sizeof(VkFramebuffer));
    result = createFramebuffers(
        device,
        renderPass,
        swapChain.extent,
        swapChain.imageViews,
        swapChain.imageCount,
        framebuffers
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create framebuffers");
    state.swapChain.framebuffers = framebuffers;

    VkCommandPool commandPool;
    result = createCommandPool(device, graphicsFamily, &commandPool);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create command pool");
    state.commandPool = commandPool;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    result = createVertexBuffer(device, state.physicalDevice, &vertexBuffer, &vertexBufferMemory);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create vertex buffer");
    state.vertexBuffer = vertexBuffer;
    state.vertexBufferMemory = vertexBufferMemory;

    VkCommandBuffer *commandBuffers = malloc(sizeof(VkCommandBuffer) * maxFramesInFlight);
    result = createCommandBuffers(device, commandPool, &commandBuffers, maxFramesInFlight);
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create command buffer");
    state.commandBuffers = commandBuffers;

    VkSemaphore *imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * maxFramesInFlight);
    VkSemaphore *renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * maxFramesInFlight);
    VkFence *inFlightFences = malloc(sizeof(VkFence) * maxFramesInFlight);
    result = createSyncObjects(
        device,
        &imageAvailableSemaphores,
        &renderFinishedSemaphores,
        &inFlightFences
    );
    RETURN_IF_NOT_VK_SUCCESS(result, "Failed to create sync objects");
    state.imageAvailableSemaphores = imageAvailableSemaphores;
    state.renderFinishedSemaphores = renderFinishedSemaphores;
    state.inFlightFences = inFlightFences;

    fprintf(stderr, "Vulkan context initialized successfully\n");
    return VK_SUCCESS;
}

void drawFrame(void) {
    vkWaitForFences(state.device, 1, &state.inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        state.device,
        state.swapChain.vkSwapChain,
        UINT64_MAX,
        state.imageAvailableSemaphores[state.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        const char *result_str = string_VkResult(result);
        fprintf(stderr, "Recreating swap chain. Reason: %s\n", result_str);
        recreateSwapChain(
            state.physicalDevice,
            state.device,
            state.windowSurface,
            state.graphicsFamily,
            state.presentFamily,
            state.renderPass,
            maxFramesInFlight,
            state.inFlightFences,
            &state.swapChain
        );
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        const char *result_str = string_VkResult(result);
        fprintf(stderr, "Result: %s\n", result_str);
        PANIC_IF_NOT_VK_SUCCESS(result, "Failed to acquire swap chain image");
    }

    vkResetFences(state.device, 1, &state.inFlightFences[state.currentFrame]);

    vkResetCommandBuffer(state.commandBuffers[state.currentFrame], 0);
    result = recordCommandBuffer(
        state.commandBuffers[state.currentFrame],
        state.vertexBuffer,
        imageIndex,
        state.renderPass,
        state.swapChain.framebuffers,
        state.swapChain.extent,
        state.graphicsPipeline
    );

    if (result != VK_SUCCESS) {
        const char *result_str = string_VkResult(result);
        fprintf(stderr, "Result: %s\n", result_str);
        PANIC_IF_NOT_VK_SUCCESS(result, "Failed to record command buffer");
    }

    VkSemaphore waitSemaphores[] = { state.imageAvailableSemaphores[state.currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { state.renderFinishedSemaphores[state.currentFrame] };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = state.commandBuffers,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    result = vkQueueSubmit(
        state.deviceQueue,
        1,
        &submitInfo,
        state.inFlightFences[state.currentFrame]
    );

    if (result != VK_SUCCESS) {
        const char *result_str = string_VkResult(result);
        fprintf(stderr, "Result: %s\n", result_str);
        PANIC_IF_NOT_VK_SUCCESS(result, "Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = { 0 };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { state.swapChain.vkSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(state.presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state.framebufferResized) {
        if (state.framebufferResized) {
            state.framebufferResized = false;
            fprintf(stderr, "Recreating swap chain. Reason: Framebuffer resized\n");
        } else {
            const char *result_str = string_VkResult(result);
            fprintf(stderr, "Recreating swap chain. Reason: %s\n", result_str);
        }

        recreateSwapChain(
            state.physicalDevice,
            state.device,
            state.windowSurface,
            state.graphicsFamily,
            state.presentFamily,
            state.renderPass,
            maxFramesInFlight,
            state.inFlightFences,
            &state.swapChain
        );
    } else if (result != VK_SUCCESS) {
        const char *result_str = string_VkResult(result);
        fprintf(stderr, "Result: %s\n", result_str);
        PANIC_IF_NOT_VK_SUCCESS(result, "Failed to present swap chain image");
    }

    state.currentFrame = (state.currentFrame + 1) % maxFramesInFlight;
}

void vulkanCleanup(void) {
    fprintf(stderr, "Cleaning up Vulkan\n");
    if (ENABLE_VALIDATION_LAYERS) {
        VkResult result = cleanupDebugMessenger(
            state.instance,
            &state.debugMessenger
        );
        UNUSED_INTENTIONAL(result);
    }

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        vkDestroySemaphore(state.device, state.renderFinishedSemaphores[i], NULL);
        vkDestroySemaphore(state.device, state.imageAvailableSemaphores[i], NULL);
        vkDestroyFence(state.device, state.inFlightFences[i], NULL);
    }
    free(state.renderFinishedSemaphores);
    free(state.imageAvailableSemaphores);
    free(state.inFlightFences);

    vkFreeCommandBuffers(state.device, state.commandPool, 1, state.commandBuffers);
    vkDestroyCommandPool(state.device, state.commandPool, NULL);

    cleanupSwapChain(state.device, &state.swapChain);
    free(state.swapChain.framebuffers);
    free(state.swapChain.imageViews);
    free(state.swapChain.images);

    vkDestroyBuffer(state.device, state.vertexBuffer, NULL);
    vkFreeMemory(state.device, state.vertexBufferMemory, NULL);

    vkDestroyPipeline(state.device, state.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(state.device, state.pipelineLayout, NULL);
    vkDestroyRenderPass(state.device, state.renderPass, NULL);

    vkDestroyDevice(state.device, NULL);
    vkDestroySurfaceKHR(state.instance, state.windowSurface, NULL);
    vkDestroyInstance(state.instance, NULL);

    glfwDestroyWindow(state.window);
    glfwTerminate();
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    UNUSED_INTENTIONAL(scancode);
    UNUSED_INTENTIONAL(mods);
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE: {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } break;
        case GLFW_KEY_R: {
            fprintf(stderr, "Reloading shaders...\n");
            TODO("Reload shaders");
        } break;
        default: { } break;
        }
    }
}

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    UNUSED_INTENTIONAL(window);
    UNUSED_INTENTIONAL(width);
    UNUSED_INTENTIONAL(height);
    state.framebufferResized = true;
}

int main(void) {
    VkResult result = renderInit();
    if (result != VK_SUCCESS) {
        const char *result_str = string_VkResult(result);
        fprintf(stderr, "Failed to initialize Vulkan: %s\n", result_str);
        exit(1);
    }

    glfwSetKeyCallback(state.window, key_callback);
    glfwSetFramebufferSizeCallback(state.window, framebuffer_resize_callback);

    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(state.device);

    vulkanCleanup();
    exit(0);
}
