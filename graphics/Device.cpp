#include <cstring>
#include <vector>

#include "Command.h"
#include "Device.h"
#include "Synchronization.h"
#include "utils/Assert.h"

static const char *const validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

static const char *const device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

VKFN_DEF(vkGetAccelerationStructureBuildSizes);
VKFN_DEF(vkCreateAccelerationStructure);
VKFN_DEF(vkCmdBuildAccelerationStructures);
VKFN_DEF(vkGetAccelerationStructureDeviceAddress);
VKFN_DEF(vkCreateRayTracingPipelines);
VKFN_DEF(vkGetRayTracingShaderGroupHandles);
VKFN_DEF(vkDestroyAccelerationStructure);
VKFN_DEF(vkCmdTraceRays);

int32_t physical_score(VkPhysicalDevice physical, VkSurfaceKHR surface);
uint32_t physical_check_queue_family(VkPhysicalDevice physical,
                                     VkSurfaceKHR surface,
                                     VkQueueFlagBits bits);

Device::Device(std::shared_ptr<Window> window) : window_(window) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Illinois Voxel Sandbox";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Custom";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count = 0;
    const char **const glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount =
        sizeof(validation_layers) / sizeof(validation_layers[0]);
    create_info.ppEnabledLayerNames = validation_layers;

    ASSERT(vkCreateInstance(&create_info, nullptr, &instance_),
           "Couldn't create Vulkan instance.");

    ASSERT(glfwCreateWindowSurface(instance_, window_->get_window(), nullptr,
                                   &surface_),
           "Couldn't create GLFW window surface.");

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    ASSERT(device_count > 0, "No physical devices.");

    std::vector<VkPhysicalDevice> possible(device_count);
    ASSERT(
        vkEnumeratePhysicalDevices(instance_, &device_count, &possible.at(0)),
        "Couldn't enumerate all physical devices.");

    int32_t best_physical = -1;
    int32_t best_score = 0;
    for (uint32_t new_physical = 0; new_physical < device_count;
         ++new_physical) {
        const int32_t new_score =
            physical_score(possible[new_physical], surface_);
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
    ray_tracing_properties_.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    ray_tracing_properties_.pNext = &acceleration_structure_properties_;
    acceleration_structure_properties_.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    acceleration_structure_properties_.pNext = nullptr;
    vkGetPhysicalDeviceProperties2(physical_device_, &device_properties);
    std::cout << "INFO: Using device "
              << device_properties.properties.deviceName << ".\n";

    queue_family_ = physical_check_queue_family(
        physical_device_, surface_,
        (VkQueueFlagBits)(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    ASSERT(queue_family_ != 0xFFFFFFFF, "Could not find queue family.");

    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceShaderFloat16Int8Features int_eight_features{};
    int_eight_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
    int_eight_features.shaderInt8 = VK_TRUE;
    int_eight_features.pNext = nullptr;

    VkPhysicalDevice8BitStorageFeatures eight_bit_storage_features{};
    eight_bit_storage_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    eight_bit_storage_features.storageBuffer8BitAccess = VK_TRUE;
    eight_bit_storage_features.pNext = &int_eight_features;

    VkPhysicalDeviceShaderAtomicInt64Features shader_atomic_int_64_features{};
    shader_atomic_int_64_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
    shader_atomic_int_64_features.shaderBufferInt64Atomics = VK_TRUE;
    shader_atomic_int_64_features.pNext = &eight_bit_storage_features;

    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features{};
    timeline_semaphore_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    timeline_semaphore_features.timelineSemaphore = VK_TRUE;
    timeline_semaphore_features.pNext = &shader_atomic_int_64_features;

    VkPhysicalDeviceSynchronization2Features synchronization_2_features{};
    synchronization_2_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization_2_features.synchronization2 = VK_TRUE;
    synchronization_2_features.pNext = &timeline_semaphore_features;

    VkPhysicalDeviceBufferDeviceAddressFeatures
        buffer_device_address_features{};
    buffer_device_address_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.bufferDeviceAddress = VK_TRUE;
    buffer_device_address_features.pNext = &synchronization_2_features;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_features{};
    acceleration_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_features.accelerationStructure = VK_TRUE;
    acceleration_features
        .descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
    acceleration_features.pNext = &buffer_device_address_features;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features{};
    ray_tracing_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    ray_tracing_features.rayTracingPipeline = VK_TRUE;
    ray_tracing_features.pNext = &acceleration_features;

    VkPhysicalDeviceVulkan11Features vulkan_11_features{};
    vulkan_11_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_11_features.shaderDrawParameters = VK_TRUE;
    vulkan_11_features.pNext = &ray_tracing_features;

    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
    indexing_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    indexing_features.runtimeDescriptorArray = VK_TRUE;
    indexing_features.pNext = &vulkan_11_features;

    VkPhysicalDeviceFeatures2 device_features{};
    device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.features.samplerAnisotropy = VK_TRUE;
    device_features.pNext = &indexing_features;

    vkGetPhysicalDeviceFeatures2(physical_device_, &device_features);

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.pNext = &device_features;
    device_create_info.enabledExtensionCount =
        sizeof(device_extensions) / sizeof(device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = device_extensions;

    ASSERT(vkCreateDevice(physical_device_, &device_create_info, nullptr,
                          &device_),
           "Couldn't create logical device.");
    vkGetDeviceQueue(device_, queue_family_, 0, &queue_);

    VKFN_INIT(vkGetAccelerationStructureBuildSizes);
    VKFN_INIT(vkCreateAccelerationStructure);
    VKFN_INIT(vkCmdBuildAccelerationStructures);
    VKFN_INIT(vkGetAccelerationStructureDeviceAddress);
    VKFN_INIT(vkCreateRayTracingPipelines);
    VKFN_INIT(vkGetRayTracingShaderGroupHandles);
    VKFN_INIT(vkDestroyAccelerationStructure);
    VKFN_INIT(vkCmdTraceRays);
}

Device::~Device() {
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

VkInstance Device::get_instance() { return instance_; }

VkPhysicalDevice Device::get_physical_device() { return physical_device_; }

VkDevice Device::get_device() { return device_; }

VkSurfaceKHR Device::get_surface() { return surface_; }

VkQueue Device::get_queue() { return queue_; }

uint32_t Device::get_queue_family() { return queue_family_; }

void Device::submit_command(
    std::shared_ptr<Command> command,
    std::vector<std::shared_ptr<Semaphore>> wait_semaphores,
    std::vector<std::shared_ptr<Semaphore>> signal_semaphores,
    std::shared_ptr<Fence> fence) {
    const auto buffer = command->get_buffer();
    const std::vector<VkPipelineStageFlags> wait_stages(
        wait_semaphores.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    std::vector<VkSemaphore> vk_wait_semaphores;
    std::vector<VkSemaphore> vk_signal_semaphores;
    for (auto semaphore : wait_semaphores) {
        vk_wait_semaphores.push_back(semaphore->get_semaphore());
    }
    for (auto semaphore : signal_semaphores) {
        vk_signal_semaphores.push_back(semaphore->get_semaphore());
    }

    std::vector<uint64_t> timeline_wait_values;
    for (const auto semaphore : wait_semaphores) {
        if (semaphore->is_timeline()) {
            timeline_wait_values.push_back(semaphore->get_wait_value());
        } else {
            timeline_wait_values.push_back(0);
        }
    }
    std::vector<uint64_t> timeline_signal_values;
    for (const auto semaphore : signal_semaphores) {
        if (semaphore->is_timeline()) {
            timeline_signal_values.push_back(semaphore->get_signal_value());
        } else {
            timeline_signal_values.push_back(0);
        }
    }

    VkTimelineSemaphoreSubmitInfo timeline_semaphore_info{};
    timeline_semaphore_info.sType =
        VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_semaphore_info.pNext = nullptr;
    timeline_semaphore_info.waitSemaphoreValueCount =
        static_cast<uint32_t>(timeline_wait_values.size());
    timeline_semaphore_info.pWaitSemaphoreValues = timeline_wait_values.data();
    timeline_semaphore_info.signalSemaphoreValueCount =
        static_cast<uint32_t>(timeline_signal_values.size());
    timeline_semaphore_info.pSignalSemaphoreValues =
        timeline_signal_values.data();

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_semaphore_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    submit_info.waitSemaphoreCount =
        static_cast<uint32_t>(vk_wait_semaphores.size());
    submit_info.pWaitSemaphores = vk_wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.signalSemaphoreCount =
        static_cast<uint32_t>(vk_signal_semaphores.size());
    submit_info.pSignalSemaphores = vk_signal_semaphores.data();

    VkFence vk_fence = fence ? fence->get_fence() : VK_NULL_HANDLE;

    ASSERT(vkQueueSubmit(queue_, 1, &submit_info, vk_fence),
           "Unable to submit command.");
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR
Device::get_ray_tracing_properties() {
    return ray_tracing_properties_;
}

VkPhysicalDeviceAccelerationStructurePropertiesKHR
Device::get_acceleration_structure_properties() {
    return acceleration_structure_properties_;
}

uint32_t physical_check_queue_family(VkPhysicalDevice physical,
                                     VkSurfaceKHR surface,
                                     VkQueueFlagBits bits) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count,
                                             nullptr);
    ASSERT(queue_family_count > 0, "No queue families.");

    std::vector<VkQueueFamilyProperties> possible(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count,
                                             &possible.at(0));

    for (uint32_t queue_family_index = 0;
         queue_family_index < queue_family_count; ++queue_family_index) {
        if ((possible.at(queue_family_index).queueFlags & bits) == bits) {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, queue_family_index,
                                                 surface, &present_support);
            if (present_support == VK_TRUE) {
                return queue_family_index;
            }
        }
    }

    return 0xFFFFFFFF;
}

int32_t physical_check_extensions(VkPhysicalDevice physical) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical, nullptr, &extension_count,
                                         nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical, nullptr, &extension_count,
                                         &available_extensions[0]);

    uint32_t required_extension_index;
    for (required_extension_index = 0;
         required_extension_index <
         sizeof(device_extensions) / sizeof(device_extensions[0]);
         ++required_extension_index) {
        uint32_t available_extension_index;
        for (available_extension_index = 0;
             available_extension_index < extension_count;
             ++available_extension_index) {
            if (!strcmp(available_extensions.at(available_extension_index)
                            .extensionName,
                        device_extensions[required_extension_index])) {
                break;
            }
        }
        if (available_extension_index >= extension_count) {
            break;
        }
    }
    if (required_extension_index <
        sizeof(device_extensions) / sizeof(device_extensions[0])) {
        return -1;
    } else {
        return 0;
    }
}

int32_t physical_check_features_support(VkPhysicalDevice physical) {
    VkPhysicalDeviceShaderFloat16Int8Features int_eight_features{};
    int_eight_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
    int_eight_features.pNext = nullptr;

    VkPhysicalDevice8BitStorageFeatures eight_bit_storage_features{};
    eight_bit_storage_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    eight_bit_storage_features.pNext = &int_eight_features;

    VkPhysicalDeviceShaderAtomicInt64Features shader_atomic_int_64_features{};
    shader_atomic_int_64_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
    shader_atomic_int_64_features.pNext = &eight_bit_storage_features;

    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features{};
    timeline_semaphore_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    timeline_semaphore_features.pNext = &shader_atomic_int_64_features;

    VkPhysicalDeviceSynchronization2Features synchronization_2_features{};
    synchronization_2_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization_2_features.pNext = &timeline_semaphore_features;

    VkPhysicalDeviceBufferDeviceAddressFeatures
        buffer_device_address_features{};
    buffer_device_address_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    buffer_device_address_features.pNext = &synchronization_2_features;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_features{};
    acceleration_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_features.pNext = &buffer_device_address_features;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_features{};
    ray_tracing_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    ray_tracing_features.pNext = &acceleration_features;

    VkPhysicalDeviceVulkan11Features vulkan_11_features{};
    vulkan_11_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan_11_features.pNext = &ray_tracing_features;

    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
    indexing_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing_features.pNext = &vulkan_11_features;

    VkPhysicalDeviceFeatures2 device_features{};
    device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.features.samplerAnisotropy = VK_TRUE;
    device_features.pNext = &indexing_features;

    vkGetPhysicalDeviceFeatures2(physical, &device_features);

    if (indexing_features.descriptorBindingPartiallyBound &&
        indexing_features.runtimeDescriptorArray &&
        vulkan_11_features.shaderDrawParameters &&
        ray_tracing_features.rayTracingPipeline &&
        acceleration_features.accelerationStructure &&
        acceleration_features
            .descriptorBindingAccelerationStructureUpdateAfterBind &&
        buffer_device_address_features.bufferDeviceAddress &&
        synchronization_2_features.synchronization2 &&
        timeline_semaphore_features.timelineSemaphore &&
        shader_atomic_int_64_features.shaderBufferInt64Atomics &&
        eight_bit_storage_features.storageBuffer8BitAccess &&
        int_eight_features.shaderInt8) {
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

    const uint32_t queue_check = physical_check_queue_family(
        physical, surface,
        (VkQueueFlagBits)(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
    if (queue_check == 0xFFFFFFFF) {
        return -1;
    }

    const int32_t extension_check = physical_check_extensions(physical);
    if (extension_check == -1) {
        return -1;
    }

    uint32_t num_formats, num_present_modes;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &num_formats,
                                         nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface,
                                              &num_present_modes, nullptr);
    if (num_formats == 0 || num_present_modes == 0) {
        return -1;
    }

    const int32_t features_check = physical_check_features_support(physical);
    if (features_check == -1) {
        return -1;
    }

    return device_type_score;
}
