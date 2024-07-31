#include <stdint.h>
#include <time.h>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "ext.h"
#include "helper.h"
#include "vert.h"
#include "frag.h"

struct QueueFamilyIndices {
    bool hasGraphics;
    uint32_t graphics;
    bool hasPresent;
    uint32_t present;
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    uint32_t formatCount;
    VkPresentModeKHR* presentModes;
    uint32_t modeCount;
};

void freeSwapchainSupportDetailsStruct(struct SwapchainSupportDetails* details) {
    free(details->formats);
    free(details->presentModes);
    details->modeCount = 0;
    details->formatCount = 0;
}

#define MAX_FRAMES_IN_FLIGHT 2
uint32_t currentFrame = 0;
VkQueue presentQueue;
VkQueue graphicsQueue;
VkSwapchainKHR swapchain;
VkImage* swapchainImages;
uint32_t swapchainImageCount;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;
VkImageView* swapchainImageViews;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkFramebuffer* swapchainFramebuffers;
VkCommandPool commandPool;
VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

const float vertexData[] = {
    // first triangle
    0.0f, -0.5f,    1.0f, 0.0f, 0.0f,
    0.5f, 0.5f,     0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
    // second triangle 
    0.4f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.9f, 0.3f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.2f, 0.0f, 1.0f, 1.0f,
    // second triangle
    0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    1.0f, 0.5f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.5f, 0.0f, 1.0f, 1.0f,
};

VkVertexInputBindingDescription getVertexDataBindingDescription() {
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float) * 5;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

VkVertexInputAttributeDescription* getVertexAttributeDescriptions() {
    VkVertexInputAttributeDescription* attributeDescriptions = 
             malloc(sizeof(VkVertexInputAttributeDescription) * 2);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float) * 2;
    return attributeDescriptions; 
}

// Functions:
void initWindow();
void initVulkan();
void createInstance();
void getRequiredExtensions(uint32_t* extensionCount, const char** extensions);
bool validationLayersSupported();
void setupDebugMessenger();
void createSurface();
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);
static VKAPI_ATTR VkBool32 VKAPI_CALL 
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
void pickPhysicalDevice();
void findQueueFamilies(VkPhysicalDevice device, struct QueueFamilyIndices* familyIndices);
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
void querySwapchainSupport(VkPhysicalDevice device, struct SwapchainSupportDetails* details);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const struct SwapchainSupportDetails* 
                                           swapchainSupportDetails);
VkPresentModeKHR chooseSwapPresentMode(const struct SwapchainSupportDetails*
                                        swapchainSupportDetails);
VkExtent2D chooseSwapExtent(const struct SwapchainSupportDetails*
                            swapchainSupportDetails);
void createLogicalDevice();
void recreateSwapchain();
void createSwapchain();
void createImageViews();
void createRenderPass();
void createGraphicsPipeline();
void cleanupSwapchain();
VkShaderModule createShaderModule(char* code, uint32_t size);
void createFramebuffers();
void createCommandPool();
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                  VkBuffer* buffer, VkDeviceMemory* deviceMemory);
void createVertexBuffers();
void createCommandBuffers();
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
void createSyncObjects();
void mainLoop();
void drawFrame();
void cleanup();
void framebufferResized(GLFWwindow* window, int width, int height);

// Variables:
GLFWwindow* window;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* requiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME   
};
const uint32_t requiredDeviceExtensionCount = 1;

VkInstance instance;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
VkSurfaceKHR surface;

// Debug variables:
#ifdef NDEBUG
    const bool validationLayersEnabled = false;
    const char* requiredValidationLayers[] = {};
    const uint32_t requiredValidationLayersCount = 0;
    const char* requiredValidationExtensions[] = {};
    const uint32_t requiredValidationExtensionCount = 0;
#else
    const bool validationLayersEnabled = true;
    const char* requiredValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    const uint32_t requiredValidationLayersCount = 1;
    const char* requiredValidationExtensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    const uint32_t requiredValidationExtensionCount = 1;
#endif
VkDebugUtilsMessengerEXT debugMessenger;

// Main:
int main() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}
void initWindow() {
    if(!glfwInit()) {
        fprintf(stderr, "glfwInit() failed, aborting");
        EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
    if(window == NULL) {
        fprintf(stderr, "Failed to create window, aborting.");
        EXIT_FAILURE;
    }
    glfwSetWindowSizeLimits(window,256, 256, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetFramebufferSizeCallback(window, framebufferResized);
}

void initVulkan() {
    createInstance();
    if(validationLayersEnabled) setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffers();
    createCommandBuffers();
    createSyncObjects();
}


void createInstance() {
    
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    //Find out what extensions the hardware supports:
    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL);
    VkExtensionProperties availableExtensionProperties[availableExtensionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensionProperties);
 
    //Find out what extensions glfw needs:
    uint32_t requiredExtensionCount = 0;
    getRequiredExtensions(&requiredExtensionCount, NULL);
    const char* requiredExtensions[requiredExtensionCount];
    getRequiredExtensions(&requiredExtensionCount, requiredExtensions);
    printf("%-50s %-15s %-15s\n", "Extension Name", "Required", 
        "Available");

    for(int i = 0; i < requiredExtensionCount; i++) {
        const char* requiredExtensionName = requiredExtensions[i];
        bool isAvailable = false;
        for(int j = 0; j < availableExtensionCount; j++) {
            if(strcmp(requiredExtensionName, availableExtensionProperties[j].extensionName) == 0) {
                isAvailable = true;
                break;
            }
        }
        char availableIndicator = 'O';
        if(isAvailable) availableIndicator = 'X';
        printf("%-50s %-15c %-15c\n",  requiredExtensionName, 
            'X', availableIndicator);
    }
    for(int i = 0; i < availableExtensionCount; i++) {
        const char* availableExtensionName = availableExtensionProperties[i].extensionName;
        bool alreadyEvaluated = false;
        for(int j = 0; j < requiredExtensionCount; j++) {
            if(strcmp(availableExtensionName, requiredExtensions[j]) == 0) {
                alreadyEvaluated = true;
                break;
            }
        }
        if(alreadyEvaluated) {
            continue;
        }
        printf("%-50s %-15c %-15c\n",  availableExtensionName, 
            'O', 'X'); 
    }
    printf("\n");

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = requiredExtensionCount;
    createInfo.ppEnabledExtensionNames = (const char**)requiredExtensions;

    if(!validationLayersSupported() && validationLayersEnabled) {
        fprintf(stderr, "Validation layers enabled and not supported!, aborting.");
        EXIT_FAILURE;
    }
    createInfo.enabledLayerCount = requiredValidationLayersCount;
    createInfo.ppEnabledLayerNames = requiredValidationLayers;
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    if(validationLayersEnabled) {
        populateDebugMessengerCreateInfo(&debugMessengerCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerCreateInfo;
    }

    if(vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance() failed, aborting.");
        EXIT_FAILURE;
    }

}

void getRequiredExtensions(uint32_t* extensionCount, const char** extensions) {
    uint32_t glfwRequiredExtensionCount;
    const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    if(extensions == NULL) {
        if(validationLayersEnabled) {
            *extensionCount = glfwRequiredExtensionCount + requiredValidationExtensionCount;
        }else {
            *extensionCount = glfwRequiredExtensionCount;
        }
        return;
    }
    
    int index = 0;
    for(int i = 0; i < glfwRequiredExtensionCount; i++) {
        extensions[index] = glfwRequiredExtensions[i];
        index++;
    }
    if(!validationLayersEnabled) return;
    for(int i = 0; i < requiredValidationExtensionCount; i++) {
        extensions[index] = requiredValidationExtensions[i];
        index++;
    }
}
bool validationLayersSupported() {
    bool validationLayersSupported = true;
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    VkLayerProperties availableLayerProperties[availableLayerCount];
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayerProperties);

    printf("%-50s %-15s %-15s\n", "Layer Name", "Required", 
       "Available");
    for(int i = 0; i < requiredValidationLayersCount; i++) {
        const char* requiredValidationLayerName = requiredValidationLayers[i];
        bool isAvailable = false;
        for(int j = 0; j < availableLayerCount; j++) {
            if(strcmp(requiredValidationLayerName, availableLayerProperties[j].layerName) == 0) {
                isAvailable = true;
                break;
            }
        }
        char availableIndicator = 'O';
        if(isAvailable) {
            availableIndicator = 'X';
        }else {
            validationLayersSupported = false;
        }
        printf("%-50s %-15c %-15c\n", requiredValidationLayerName, 'X', availableIndicator);           

    }
    for(int i = 0; i < availableLayerCount; i++) {
        const char* availableLayerName = availableLayerProperties[i].layerName;
        bool alreadyEvaluated = false;
        for(int j = 0; j < requiredValidationLayersCount; j++) {
            if(strcmp(availableLayerName, requiredValidationLayers[j]) == 0) {
                alreadyEvaluated = true;
                break;
            }
        }
        if(alreadyEvaluated) continue; printf("%-50s %-15c %-15c\n", availableLayerName, 'O', 'X');           
    }
    printf("\n");
    
    return validationLayersSupported;
}
static VKAPI_ATTR VkBool32 VKAPI_CALL 
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "Validation Layers: %s\n", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

void setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    populateDebugMessengerCreateInfo(&createInfo); 
   
    if(CreateDebugUtilsMessengerEXT(instance, &createInfo, NULL, &debugMessenger) != VK_SUCCESS) {
        fprintf(stderr, "CreateDebugUtilsMessengerEXT failed, aborting.");
        EXIT_FAILURE;
    }

}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
    createInfo->sType =  VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
    createInfo->pUserData = NULL;
    createInfo->flags = 0;
}

void createSurface() {
    if(glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        fprintf(stderr, "glfwCreateWindowSurface failed, aborting");
        EXIT_FAILURE;
    }
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if(deviceCount == 0) {
        fprintf(stderr, "No physical devices available, aborting.");
        EXIT_FAILURE;
    }
    VkPhysicalDevice availablePhysicalDevices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices);   
    
    printf("GPU Names\n");
    for(int i = 0; i < deviceCount; i++) {
        VkPhysicalDevice availableDevice = availablePhysicalDevices[i];
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(availableDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(availableDevice, &deviceFeatures);
        printf("Device Index: %d\n", i);
        printf("\tDevice Name: %s\n", deviceProperties.deviceName);
        printf("\tMax Texture Size: %d\n", deviceProperties.limits.maxImageDimension2D);
        
        struct QueueFamilyIndices queueFamilyIndices;
        findQueueFamilies(availableDevice, &queueFamilyIndices);
        bool hasExtensions = checkDeviceExtensionSupport(availableDevice);
        if(!hasExtensions) {
            printf("%s does not have required device extensions.\n\n", deviceProperties.deviceName);
            continue;
        }
        struct SwapchainSupportDetails swapchainSupportDetails;
        querySwapchainSupport(availableDevice, &swapchainSupportDetails);
        bool swapchainHasSupport = (swapchainSupportDetails.formatCount > 0) && 
                                   (swapchainSupportDetails.presentModes > 0);
        freeSwapchainSupportDetailsStruct(&swapchainSupportDetails); 

        if(deviceFeatures.geometryShader && queueFamilyIndices.hasGraphics && 
            queueFamilyIndices.hasPresent && hasExtensions) {
            physicalDevice = availableDevice;
            printf("%s is suitable.\n\n", deviceProperties.deviceName);
        }
    }

    if(physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find suitable GPU, aborting.");
        EXIT_FAILURE;
    }
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    printf("Chose %s.\n", deviceProperties.deviceName);
}

void findQueueFamilies(VkPhysicalDevice device, struct QueueFamilyIndices* familyIndices) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);
    for(int i = 0; i < queueFamilyCount; i++) {
        VkQueueFamilyProperties queueFamProps = queueFamilyProperties[i];
        if(queueFamProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            familyIndices->hasGraphics = true;
            familyIndices->graphics = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport) {
            familyIndices->hasPresent = true; 
            familyIndices->present = i;
        }
    }
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    bool hasSupport = true;
    uint32_t deviceExtensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, NULL);
    VkExtensionProperties deviceExtensionProperties[deviceExtensionCount];
    vkEnumerateDeviceExtensionProperties(device, NULL, &deviceExtensionCount, 
                                         deviceExtensionProperties);
    printf("\t%-50s %-15s %-15s\n", "Extension Name", "Required", 
        "Available");
    for(int i = 0; i < requiredDeviceExtensionCount; i++) {
        bool available = false;
        for(int j = 0; j < deviceExtensionCount; j++){
            if(strcmp(requiredDeviceExtensions[i], 
                      deviceExtensionProperties[j].extensionName) == 0) {
                available = true;
            }
                     
        }
        char availableIndicator = 'X';
        if(available == false) {
            hasSupport = false;
            availableIndicator = 'O';
        }
        printf("\t%-50s %-15c %-15c\n", requiredDeviceExtensions[i], 'X', availableIndicator);  
    }
    for(int i = 0; i < deviceExtensionCount; i++) {
        bool required = false;
        for(int j = 0; j < requiredDeviceExtensionCount; j++) {
            if(strcmp(requiredDeviceExtensions[j], 
                      deviceExtensionProperties[i].extensionName) == 0) {
                required = true;
                break;
            }
        }
        if(required) continue;
        printf("\t%-50s %-15c %-15c\n", deviceExtensionProperties[i].extensionName,
                'O', 'X');           
    }
    return hasSupport;
}


void querySwapchainSupport(VkPhysicalDevice physicalDevice, struct SwapchainSupportDetails* details) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &(details->capabilities));

    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &(details->formatCount), NULL);
    details->formats = malloc(sizeof(VkSurfaceFormatKHR) * details->formatCount);
    if(details->formats == NULL) {
        fprintf(stderr, "malloc returned NULL, aborting");
        EXIT_FAILURE;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &(details->formatCount), details->formats);

    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &(details->modeCount), NULL);
    details->presentModes = malloc(sizeof(VkPresentModeKHR) * details->modeCount);
    if(details->presentModes == NULL) {
        fprintf(stderr, "malloc returned NULL, aborting");
        EXIT_FAILURE;
    }
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &(details->modeCount), details->presentModes);
}
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const struct SwapchainSupportDetails* 
                                           swapchainSupportDetails) {
    for(int i = 0; i < swapchainSupportDetails->formatCount; i++) {
        if(swapchainSupportDetails->formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
            swapchainSupportDetails->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return swapchainSupportDetails->formats[i];
        }
    }
    return swapchainSupportDetails->formats[0];
}
VkPresentModeKHR chooseSwapPresentMode(const struct SwapchainSupportDetails*
                                        swapchainSupportDetails) {
    for(int i = 0; i < swapchainSupportDetails->modeCount; i++) {
        if(swapchainSupportDetails->presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return swapchainSupportDetails->presentModes[i];    
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D chooseSwapExtent(const struct SwapchainSupportDetails*
                            swapchainSupportDetails) {
    if(swapchainSupportDetails->capabilities.currentExtent.width != UINT32_MAX) {
        return swapchainSupportDetails->capabilities.currentExtent;
    }else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = {
            (uint32_t) width,
            (uint32_t) height
        };
        actualExtent.width = clamp(actualExtent.width, 
                                   swapchainSupportDetails->capabilities.minImageExtent.width,
                                   swapchainSupportDetails->capabilities.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, 
                                   swapchainSupportDetails->capabilities.minImageExtent.height,
                                   swapchainSupportDetails->capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void createLogicalDevice() {
    struct QueueFamilyIndices queueFamIndices;
    findQueueFamilies(physicalDevice, &queueFamIndices);
    uint32_t requiredFamilyCount = 0;
    if(queueFamIndices.present == queueFamIndices.graphics) {
        requiredFamilyCount = 1;
    }else {
        requiredFamilyCount = 2;
    }
     
    VkDeviceQueueCreateInfo queueCreateInfos[requiredFamilyCount];
    for(int i = 0; i < requiredFamilyCount; i++) {
        queueCreateInfos[i].flags = 0;
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
        queueCreateInfos[i].pNext = NULL;
        switch(i) {
            case(0):
                queueCreateInfos[i].queueFamilyIndex = queueFamIndices.graphics;
                break;
            case(1):
                queueCreateInfos[i].queueFamilyIndex = queueFamIndices.present;
                break;
        }
    }
    
    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = requiredFamilyCount;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledLayerCount = requiredValidationLayersCount;
    createInfo.ppEnabledLayerNames = requiredValidationLayers;
    
    createInfo.enabledExtensionCount = requiredDeviceExtensionCount;
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions;

    if(vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device, aborting.");
        EXIT_FAILURE;
    }

    vkGetDeviceQueue(device, queueFamIndices.graphics, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamIndices.present, 0, &presentQueue);

}
void createSwapchain() {
    struct SwapchainSupportDetails swapchainSupportDetails;
    querySwapchainSupport(physicalDevice, &swapchainSupportDetails);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(&swapchainSupportDetails);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(&swapchainSupportDetails);
    VkExtent2D extent = chooseSwapExtent(&swapchainSupportDetails);
    
    uint32_t imageCount = swapchainSupportDetails.capabilities.minImageCount + 1;
    if(swapchainSupportDetails.capabilities.maxImageCount > 0 && 
        imageCount > swapchainSupportDetails.capabilities.maxImageCount)  {
        imageCount = swapchainSupportDetails.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType =  VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
    struct QueueFamilyIndices queueFamilyIndices = {};
    findQueueFamilies(physicalDevice, &queueFamilyIndices);
    uint32_t indices[] = {queueFamilyIndices.graphics, queueFamilyIndices.present};
    if(queueFamilyIndices.present != queueFamilyIndices.graphics) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    }else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
    }
    createInfo.preTransform = swapchainSupportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if(vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateSwapchainKHR did not return VK_SUCCESS, aborting.");
        EXIT_FAILURE;
    }
    
    //Set global variables
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
    swapchainImages = malloc(sizeof(VkImage) * swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    freeSwapchainSupportDetailsStruct(&swapchainSupportDetails);
}
void createImageViews() {
    swapchainImageViews = malloc(sizeof(VkImage) * swapchainImageCount); 
    for(int i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if(vkCreateImageView(device, &createInfo, NULL, &swapchainImageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image views, aborting.");
            EXIT_FAILURE;
        }
    }
}
void createRenderPass() {
    VkAttachmentDescription colorAttachment={};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef={};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass={};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
     VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassInfo={};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1; //CHANGEW TO !
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateRenderPass failed, aborting.");
        EXIT_FAILURE; 
    }

}
void createGraphicsPipeline() { 
    VkShaderModule vertShaderModule = createShaderModule((char*)vertShaderByteCode, sizeof(vertShaderByteCode));
    VkShaderModule fragShaderModule = createShaderModule((char*)fragShaderByteCode, sizeof(fragShaderByteCode));

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
   
    uint32_t dynamicStateCount = 2; 
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR 
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStateCount;
    dynamicState.pDynamicStates = (VkDynamicState*)&dynamicStates;
    
    VkVertexInputBindingDescription description = getVertexDataBindingDescription();
    VkVertexInputAttributeDescription* attribs = getVertexAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo ={};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &description;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attribs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    
    VkOffset2D offset = {0,0};
    VkRect2D scissor = {};
    scissor.offset  = offset;
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling ={};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment={};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT 
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending={};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo={};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = NULL; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = NULL; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr,"vkCreatePipelineLayout failed, aborting.");
        EXIT_FAILURE;
    }
    VkGraphicsPipelineCreateInfo pipelineInfo= {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline)
       != VK_SUCCESS) {
        fprintf(stderr, "vkCreateGraphicsPipelines failed, aborting.");
        EXIT_FAILURE;
    }

    vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);
    free(attribs);
}
VkShaderModule createShaderModule(char* code, uint32_t size) {
    VkShaderModuleCreateInfo createInfo= {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = (uint32_t*)code;
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateShaderModule failed, aborting.");
        EXIT_FAILURE;
    }
    return shaderModule;
   
}
void createFramebuffers() {
    swapchainFramebuffers = malloc(sizeof(VkFramebuffer) * swapchainImageCount);
    for(int i = 0; i < swapchainImageCount; i++) {
        VkImageView attachments = swapchainImageViews[i];
        VkFramebufferCreateInfo framebufferInfo = {};

        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if(vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapchainFramebuffers[i]) != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateFramebuffer failed, aborting.");
            EXIT_FAILURE;
        }
    }


}
void createCommandPool() {
    struct QueueFamilyIndices queueFamilyIndices;
    findQueueFamilies(physicalDevice, &queueFamilyIndices);
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
    if(vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        fprintf(stderr, "ckCreateCommandPool failed, aborting.");
        EXIT_FAILURE;
    }
}
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                  VkBuffer* buffer, VkDeviceMemory* deviceMemory) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer, aborting.");
        exit(EXIT_FAILURE);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(device, &allocInfo, NULL, deviceMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate vertex buffer memory, aborting.");
        exit(EXIT_FAILURE);
    }

    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

}
void createVertexBuffers() {
    VkDeviceSize bufferSize = sizeof(vertexData);
    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &vertexBuffer, &vertexBufferMemory);
       void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertexData, bufferSize);
    vkUnmapMemory(device, vertexBufferMemory);

}
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if(typeFilter & (1 << i) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    fprintf(stderr, "Failed to find suitable memory!");
    return -1;
}
void createCommandBuffers() {

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        fprintf(stderr, "vkAllocateCommandBuffers failed, aborting");
        EXIT_FAILURE; 
    }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = NULL;

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "vkBeginComandBuffer failed, aborting");
        EXIT_FAILURE;
    }

         VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    VkOffset2D offset = {0, 0};
    renderPassInfo.renderArea.offset = offset;
    renderPassInfo.renderArea.extent = swapchainExtent;

    VkClearValue clearColor = {};
    VkClearColorValue color= {0.0, 0.0, 0.0, 1.0};
    clearColor.color = color;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;


    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); 
    VkViewport viewport={};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)(swapchainExtent.width);
    viewport.height = (float)(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor={};

    scissor.offset = offset;
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdDraw(commandBuffer, sizeof(vertexData)/(sizeof(float)*5), 1, 0, 0);
    
    vkCmdEndRenderPass(commandBuffer);
    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "vkEndCommandBuffer() failed, aborting");
        EXIT_FAILURE;
    }
}
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        if(vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) 
            != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) 
            != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS) {

            fprintf(stderr, "vkCreateSempaphore or vkCreateFence failed, aborting.");
            EXIT_FAILURE; 
        }
    }
}
void recreateSwapchain() {
    
    vkDeviceWaitIdle(device);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    while(width == 0 || height == 0) {
        return;
    }
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createFramebuffers();
}

void cleanupSwapchain() {
     for(int i =0; i < swapchainImageCount; i++) {
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], NULL);
    }
    free(swapchainFramebuffers);
    for(int i = 0; i < swapchainImageCount; i++) {
            vkDestroyImageView(device, swapchainImageViews[i], NULL);
    }
    free(swapchainImageViews);
    free(swapchainImages);
    vkDestroySwapchainKHR(device, swapchain, NULL);

}
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}

void drawFrame() {
    

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                         &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        fprintf(stderr, "OUT OF DATE in draw()");
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "vkAcquireNextImageKHR failed, aborting.");
        EXIT_FAILURE;
    }
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount =1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];
    if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        fprintf(stderr, "vkQueueSubmit failed, aborting.");
        EXIT_FAILURE;
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Resizing swapchain in draw()");
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        fprintf(stderr, "vkQueuePresentKHR failed, aborting.");
        EXIT_FAILURE;
    }
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void cleanup() {
    cleanupSwapchain();
    vkDestroyBuffer(device, vertexBuffer, NULL);
    vkFreeMemory(device, vertexBufferMemory, NULL);
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
        vkDestroyFence(device, inFlightFences[i], NULL);
    }
    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    if(validationLayersEnabled) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    }
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void framebufferResized(GLFWwindow* window, int width, int height) {
    recreateSwapchain();
    drawFrame();
}
