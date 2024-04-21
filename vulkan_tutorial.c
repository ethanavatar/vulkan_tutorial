#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include <stdint.h>

#define STRINGIFY(x) #x
#define UNUSED(x) {\
    fprintf(stderr, "UNUSED VARIABLE: %s\n", STRINGIFY(x));\
    (void) (x); }

const uint32_t initialWindowWidth = 800;
const uint32_t initialWindowSize = 600;

static struct RenderState {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    GLFWwindow* window;
} state;

#define REQUESTED_VALIDATION_LAYERS 1
const char* validationLayers[REQUESTED_VALIDATION_LAYERS] = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
#    define ENABLE_VALIDATION_LAYERS false
#else
#    define ENABLE_VALIDATION_LAYERS true
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *const pCallbackData,
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
    VkLayerProperties *availableLayers = malloc(layerCount * sizeof(VkLayerProperties));
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

const char *const *getRequiredExtensions(uint32_t *glfwExtensionCount) {
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    return glfwExtensions;
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
    const char** glfwExtensions = getRequiredExtensions(&glfwExtensionCount);

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
    const char *const name,
    PFN_vkVoidFunction *function
) {
    *function = vkGetInstanceProcAddr(state.instance, name);
    if (*function == NULL) {
        fprintf(stderr, "Failed to load extension function: %s\n", name);
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return VK_SUCCESS;
}

void initDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) {
        fprintf(stderr, "setupDebugMessenger: Validation layers not enabled\n");
        return;
    }

    fprintf(stderr, "Setting up debug messenger\n");

    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { 0 };
    fillDebugMessengerCreateInfo(&messengerInfo);

    PFN_vkCreateDebugUtilsMessengerEXT func = NULL;
    VkResult result = getExtensionFunction("vkCreateDebugUtilsMessengerEXT", (PFN_vkVoidFunction*) &func);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to load vkCreateDebugUtilsMessengerEXT\n");
        return;
    }

    result = func(state.instance, &messengerInfo, NULL, &state.debugMessenger);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create debug messenger\n");
        return;
    }

    fprintf(stderr, "Debug messenger created\n");
}

void cleanupDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) {
        fprintf(stderr, "cleanupDebugMessenger: Validation layers not enabled\n");
        return;
    }

    PFN_vkDestroyDebugUtilsMessengerEXT func;
    VkResult result = getExtensionFunction("vkDestroyDebugUtilsMessengerEXT", (PFN_vkVoidFunction*) &func);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to load vkDestroyDebugUtilsMessengerEXT\n");
        return;
    }

    func(state.instance, state.debugMessenger, NULL);
    fprintf(stderr, "Debug messenger destroyed\n");
}

int scoreDeviceCapabilities(
    VkPhysicalDevice device
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

    uint32_t queueFamilyIndex;
    VkResult result;
    result = getQueueFamilyWithFlags(device, VK_QUEUE_GRAPHICS_BIT, &queueFamilyIndex);
    if (result != VK_SUCCESS) {
        return 0;
    }

    UNUSED(queueFamilyIndex);

    return score;
}

VkResult getPhysicalDevice(
    VkPhysicalDevice *physicalDevice
) {
    physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(state.instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        fprintf(stderr, "No physical devices found\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(state.instance, &deviceCount, devices);

    // Find the device with the highest score
    int bestScore = 0;
    int bestIndex = -1;
    for (uint32_t i = 0; i < deviceCount; i++) {
        int score = scoreDeviceCapabilities(devices[i]);
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestIndex == -1) {
        fprintf(stderr, "No suitable physical device found\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    physicalDevice = devices[bestIndex];
    free(devices);

    return VK_SUCCESS;
}

VkResult getQueueFamilyWithFlags(
    VkPhysicalDevice device,
    VkQueueFlags flags,
    uint32_t *queueFamilyIndex
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & flags) {
            *queueFamilyIndex = i;
            free(queueFamilies);
            return VK_SUCCESS;
        }
    }

    free(queueFamilies);
    return VK_ERROR_INITIALIZATION_FAILED;
}

void vulkanInit() {
    fprintf(stderr, "Initializing Vulkan\n");
    VkResult result = createInstance(&state.instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        exit(1);
    }

    if (ENABLE_VALIDATION_LAYERS) initDebugMessenger(state);

    VkPhysicalDevice physicalDevice;
    result = getPhysicalDevice(&physicalDevice);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get physical device\n");
        exit(1);
    }

    UNUSED(physicalDevice);

    fprintf(stderr, "Vulkan context initialized\n");
}

void glfwWindowInit() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Triangle", NULL, NULL);
    state.window = window;
}

void vulkanCleanup() {
    fprintf(stderr, "Cleaning up Vulkan\n");
    if (ENABLE_VALIDATION_LAYERS) cleanupDebugMessenger();

    vkDestroyInstance(state.instance, NULL);

    glfwDestroyWindow(state.window);
    glfwTerminate();
}

int main(void) {
    vulkanInit();
    glfwWindowInit();

    while(!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();
    }

    vulkanCleanup();
    exit(0);
    return 0;
}
