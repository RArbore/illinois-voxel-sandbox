#include "utils/Assert.h"
#include "Swapchain.h"

std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D> choose_swapchain_options(VkPhysicalDevice physical_device, VkSurfaceCapabilitiesKHR capabilities, const std::vector<VkSurfaceFormatKHR> &formats, const std::vector<VkPresentModeKHR> &present_modes, GLFWwindow *window);

Swapchain::Swapchain(std::shared_ptr<Device> device, std::shared_ptr<Window> window) {
    uint32_t num_formats, num_present_modes;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->get_physical_device(), device->get_surface(), &num_formats, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->get_physical_device(), device->get_surface(), &num_present_modes, nullptr);

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats(num_formats);
    std::vector<VkPresentModeKHR> present_modes(num_present_modes);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->get_physical_device(), device->get_surface(), &capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->get_physical_device(), device->get_surface(), &num_formats, &formats.at(0));
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->get_physical_device(), device->get_surface(), &num_present_modes, &present_modes.at(0));

    const auto [surface_format, present_mode, swap_extent] = choose_swapchain_options(device->get_physical_device(), capabilities, formats, present_modes, window->get_window());
    swapchain_extent_ = swap_extent;

    uint32_t image_count =
	capabilities.maxImageCount > 0 && capabilities.minImageCount >= capabilities.maxImageCount ?
	capabilities.maxImageCount :
	capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = device->get_surface();
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swap_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    ASSERT(vkCreateSwapchainKHR(device->get_device(), &create_info, nullptr, &swapchain_), "Couldn't create swapchain.");

    vkGetSwapchainImagesKHR(device->get_device(), swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    swapchain_image_views_.resize(image_count);
    vkGetSwapchainImagesKHR(device->get_device(), swapchain_, &image_count, &swapchain_images_.at(0));
    VkFormat swapchain_format = surface_format.format;
    VkExtent2D swapchain_extent = swap_extent;

    VkImageSubresourceRange subresource_range {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    for (uint32_t image_index = 0; image_index < image_count; ++image_index) {
	VkImageViewCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = swapchain_images_.at(image_index);
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = swapchain_format;
	create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.subresourceRange = subresource_range;
	
	ASSERT(vkCreateImageView(device->get_device(), &create_info, nullptr, &swapchain_image_views_.at(image_index)), "Unable to create image view.");
    }

    device_ = device;
    window_ = window;
}

Swapchain::~Swapchain() {
    for (VkImageView view : swapchain_image_views_) {
	vkDestroyImageView(device_->get_device(), view, nullptr);
    }
    vkDestroySwapchainKHR(device_->get_device(), swapchain_, nullptr);
}

VkImage Swapchain::get_image(uint32_t image_index) {
    return swapchain_images_.at(image_index);
}

VkExtent2D Swapchain::get_extent() {
    return swapchain_extent_;
}

std::vector<std::shared_ptr<DescriptorSet>> Swapchain::make_image_descriptors(std::shared_ptr<DescriptorAllocator> allocator) {
    ASSERT(swapchain_images_.size() == swapchain_image_views_.size(), "Swapchain has differing numbers of images and image views.");
    std::vector<std::shared_ptr<DescriptorSet>> sets;
    for (std::size_t i = 0; i < swapchain_images_.size(); ++i) {
	VkImageView view = swapchain_image_views_.at(i);
	DescriptorSetBuilder builder(allocator);
	builder.bind_image(0, {
		.sampler = VK_NULL_HANDLE,
		.imageView = view,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	    }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	sets.emplace_back(builder.build());
    }
    return sets;
}

uint32_t Swapchain::acquire_next_image(std::shared_ptr<Semaphore> notify) {
    uint32_t index = 0;
    const VkResult result = vkAcquireNextImageKHR(device_->get_device(), swapchain_, UINT64_MAX, notify->get_semaphore(), VK_NULL_HANDLE, &index);
    ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Unable to acquire next swapchain image.");
    return index;
}

void Swapchain::present_image(uint32_t image_index, std::shared_ptr<Semaphore> wait) {
    VkSemaphore wait_sem = wait->get_semaphore();
    
    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &wait_sem;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &image_index;

    const VkResult result = vkQueuePresentKHR(device_->get_queue(), &present_info);
    ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Unable to present swapchain image.");
}

std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D> choose_swapchain_options(VkPhysicalDevice physical_device, VkSurfaceCapabilitiesKHR capabilities, const std::vector<VkSurfaceFormatKHR> &formats, const std::vector<VkPresentModeKHR> &present_modes, GLFWwindow *window) {
    VkSurfaceFormatKHR surface_format {};
    VkPresentModeKHR present_mode {};
    VkExtent2D swap_extent {};
    
    uint32_t format_index = 0;
    VkImageFormatProperties properties;
    for (; format_index < formats.size(); ++format_index) {
	if (vkGetPhysicalDeviceImageFormatProperties(physical_device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &properties) == VK_SUCCESS) {
	    surface_format = formats.at(format_index);
	    break;
	}
    }
    ASSERT(format_index < formats.size(), "ERROR: Queried surface format not found to be available.");

    present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t present_mode_index = 0; present_mode_index < present_modes.size(); ++present_mode_index) {
	if (present_modes[present_mode_index] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
	    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	else if (present_modes[present_mode_index] == VK_PRESENT_MODE_MAILBOX_KHR) {
	    present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	    break;
	}
    }

    if (capabilities.currentExtent.width != UINT32_MAX) {
	swap_extent = capabilities.currentExtent;
    }
    else {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	
	swap_extent.width = (uint32_t) width;
	swap_extent.height = (uint32_t) height;

	swap_extent.width = swap_extent.width < capabilities.minImageExtent.width ? capabilities.minImageExtent.width : swap_extent.width;
	swap_extent.height = swap_extent.height < capabilities.minImageExtent.height ? capabilities.minImageExtent.height : swap_extent.height;
	swap_extent.width = swap_extent.width > capabilities.maxImageExtent.width ? capabilities.maxImageExtent.width : swap_extent.width;
	swap_extent.height = swap_extent.height > capabilities.maxImageExtent.height ? capabilities.maxImageExtent.height : swap_extent.height;
    }

    return {surface_format, present_mode, swap_extent};
}
