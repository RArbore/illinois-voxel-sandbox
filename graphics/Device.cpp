#include <cstring>
#include <vector>

#include "utils/Assert.h"
#include "Device.h"

static const char *const validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char *const device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

int32_t physical_score(VkPhysicalDevice physical, VkSurfaceKHR surface);

Device::Device(std::shared_ptr<Window> window): window_(window) {
    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Illinois Voxel Sandbox";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Custom";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count = 0;
    const char **const glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(validation_layers[0]);
    create_info.ppEnabledLayerNames = validation_layers;

    ASSERT(vkCreateInstance(&create_info, nullptr, &instance_), "Couldn't create Vulkan instance.");

    ASSERT(glfwCreateWindowSurface(instance_, window_->get_window(), NULL, &surface_), "Couldn't create GLFW window surface.");

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, NULL);
    ASSERT(device_count > 0, "No physical devices.");

    std::vector<VkPhysicalDevice> possible(device_count);
    ASSERT(vkEnumeratePhysicalDevices(instance_, &device_count, &possible.at(0)), "Couldn't enumerate all physical devices.");

    int32_t best_physical = -1;
    int32_t best_score = 0;
    for (uint32_t new_physical = 0; new_physical < device_count; ++new_physical) {
	const int32_t new_score = physical_score(possible[new_physical], surface_);
	if (new_score > best_score) {
	    best_score = new_score;
	    best_physical = new_physical;
	}
    }

    ASSERT(best_physical != -1, "No physical device is suitable.");
    physical_device_ = possible.at(best_physical);

    VkPhysicalDeviceProperties2 device_properties;
    device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties.pNext = &ray_tracing_properties_;
    ray_tracing_properties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    ray_tracing_properties_.pNext = &acceleration_structure_properties_;
    acceleration_structure_properties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    acceleration_structure_properties_.pNext = nullptr;
    vkGetPhysicalDeviceProperties2(physical_device_, &device_properties);
    std::cout << "INFO: Using device " << device_properties.properties.deviceName << ".\n";
}

Device::~Device() {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

uint32_t physical_check_queue_family(VkPhysicalDevice physical, VkSurfaceKHR surface, VkQueueFlagBits bits) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, NULL);
    ASSERT(queue_family_count > 0, "No queue families.");

    std::vector<VkQueueFamilyProperties> possible(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, &possible.at(0));

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index) {
	if ((possible.at(queue_family_index).queueFlags & bits) == bits) {
	    VkBool32 present_support = VK_FALSE;
	    vkGetPhysicalDeviceSurfaceSupportKHR(physical, queue_family_index, surface, &present_support);
	    if (present_support == VK_TRUE) {
		return queue_family_index;
	    }
	}
    }
    
    return 0xFFFFFFFF;
}

int32_t physical_check_extensions(VkPhysicalDevice physical) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical, NULL, &extension_count, NULL);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical, NULL, &extension_count, &available_extensions[0]);

    uint32_t required_extension_index;
    for (required_extension_index = 0; required_extension_index < sizeof(device_extensions) / sizeof(device_extensions[0]); ++required_extension_index) {
	uint32_t available_extension_index;
	for (available_extension_index = 0; available_extension_index < extension_count; ++available_extension_index) {
	    if (!strcmp(available_extensions.at(available_extension_index).extensionName, device_extensions[required_extension_index])) {
		break;
	    }
	}
	if (available_extension_index >= extension_count) {
	    break;
	}
    }
    if (required_extension_index < sizeof(device_extensions) / sizeof(device_extensions[0])) {
	return -1;
    }
    else {
	return 0;
    }
}

SwapchainSupport physical_check_swapchain_support(VkPhysicalDevice specific_physical_device, VkSurfaceKHR surface) {
    uint32_t num_formats, num_present_modes;
    vkGetPhysicalDeviceSurfaceFormatsKHR(specific_physical_device, surface, &num_formats, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(specific_physical_device, surface, &num_present_modes, NULL);

    SwapchainSupport ss { {}, std::vector<VkSurfaceFormatKHR>(num_formats), std::vector<VkPresentModeKHR>(num_present_modes) };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(specific_physical_device, surface, &ss.capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(specific_physical_device, surface, &num_formats, &ss.formats.at(0));
    vkGetPhysicalDeviceSurfacePresentModesKHR(specific_physical_device, surface, &num_present_modes, &ss.present_modes.at(0));

    return ss;
}

int32_t physical_check_features_support(VkPhysicalDevice physical) {
    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features {};
    buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.pNext = NULL;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_features {};
    acceleration_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_features.pNext = &buffer_device_address_features;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features {};
    ray_tracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    ray_tracing_features.pNext = &acceleration_features;

    VkPhysicalDeviceVulkan11Features vulkan_11_features {};
    vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_11_features.pNext = &ray_tracing_features;

    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features {};
    indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing_features.pNext = &vulkan_11_features;

    VkPhysicalDeviceFeatures2 device_features {};
    device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.features.samplerAnisotropy = VK_TRUE;
    device_features.pNext = &indexing_features;
    
    vkGetPhysicalDeviceFeatures2(physical, &device_features);
    
    if (indexing_features.descriptorBindingPartiallyBound &&
	indexing_features.runtimeDescriptorArray &&
	vulkan_11_features.shaderDrawParameters &&
	ray_tracing_features.rayTracingPipeline &&
	acceleration_features.accelerationStructure &&
	acceleration_features.descriptorBindingAccelerationStructureUpdateAfterBind &&
	buffer_device_address_features.bufferDeviceAddress
	) {
	return 0;
    }
    return -1;
}

int32_t physical_score(VkPhysicalDevice physical, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical, &device_properties);

    int32_t device_type_score = 0;
    switch (device_properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
	device_type_score = 0;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
	device_type_score = 1;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
	device_type_score = 2;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
	device_type_score = 3;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
	device_type_score = 4;
	break;
    default:
	device_type_score = 0;
    }

    const uint32_t queue_check = physical_check_queue_family(physical, surface, (VkQueueFlagBits) (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    if (queue_check == 0xFFFFFFFF) {
	return -1;
    }

    const int32_t extension_check = physical_check_extensions(physical);
    if (extension_check == -1) {
	return -1;
    }

    const SwapchainSupport support_check = physical_check_swapchain_support(physical, surface);
    if (support_check.formats.size() == 0 || support_check.present_modes.size() == 0) {
	return -1;
    }

    const int32_t features_check = physical_check_features_support(physical);
    if (features_check == -1) {
	return -1;
    }

    return device_type_score;
}
