#include <cstdlib>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayer = false;
#else
const bool enableValidationLayer = true;
#endif

class HelloTriangleApp
{
public:
    GLFWwindow* window;
    
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        bool IsComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    
    void run()
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

    void init_window()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        
    }

    void init_vulkan()
    {
        create_instance();
        create_surface();
        pickPhysicalDevice();
        createLogicalDevice();
    }
    
    void create_surface()
    {
        VkResult ret = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if(ret != VK_SUCCESS)
        {
            std::cerr << "result error ="<<ret<<std::endl;
            throw std::runtime_error("failed to create window surface!, %s");
        }
    }
    
    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };
        
        float queuePriority = 1.0f;
        for(uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        VkPhysicalDeviceFeatures deviceFeature{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeature;
        createInfo.enabledExtensionCount = 0;
        createInfo.enabledLayerCount = 0;
        
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }
        
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
        if(deviceCount == 0)
        {
            throw std::runtime_error("failed to find gpus with vulkan support!");
        }
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        
        for(auto& device : devices)
        {
            if(isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }
        
        if(physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        
    }
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
        int i=0;
        for(auto& queueFamily : queueFamilies)
        {
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if(presentSupport)
            {
                indices.presentFamily = i;
            }
            
            if(indices.IsComplete())
                break;
            
            ++i;
        }

        return indices;
    }
    
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);
        
        return indices.IsComplete();
    }
    
    void create_instance()
    {
        VkApplicationInfo appInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions;
        for(uint32_t i=0; i < glfwExtensionCount; i++)
        {
            requiredExtensions.emplace_back(glfwExtensions[i]);
        }
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        if(enableValidationLayer && !checkValidationLayerSupport(validationLayers))
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        
        if(enableValidationLayer)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }
            
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
        
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        
        std::cout << "available extensions:\n";
        for(const auto& extension : extensions)
        {
            std::cout << '\t' << extension.extensionName << '\n';
        }
        
    }

    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
        for(const char* layerName : validationLayers)
        {
            bool layerFound = false;
            
            for(const auto& layerProperties : availableLayers)
            {
                if(strcmp(layerName, layerProperties.layerName))
                {
                    layerFound = true;
                    break;
                }
            }
            
            if(!layerFound)
            {
                return false;
            }
        }
        
        return true;
    }

    void main_loop()
    {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {

    HelloTriangleApp app;
    try
    {
        app.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}
