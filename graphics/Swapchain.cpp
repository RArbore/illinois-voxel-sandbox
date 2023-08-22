#include "utils/Assert.h"
#include "Swapchain.h"

std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D> choose_swapchain_options(VkSurfaceCapabilitiesKHR capabilities, const std::vector<VkSurfaceFormatKHR> &formats, const std::vector<VkPresentModeKHR> &present_modes, GLFWwindow *window);

Swapchain::Swapchain(std::shared_ptr<Device> device, std::shared_ptr<Window> window) {
    uint32_t num_formats, num_present_modes;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->get_physical_device(), device->get_surface(), &num_formats, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->get_physical_device(), device->get_surface(), &num_present_modes, NULL);

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats(num_formats);
    std::vector<VkPresentModeKHR> present_modes(num_present_modes);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->get_physical_device(), device->get_surface(), &capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->get_physical_device(), device->get_surface(), &num_formats, &formats.at(0));
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->get_physical_device(), device->get_surface(), &num_present_modes, &present_modes.at(0));

    const auto [surface_format, present_mode, swap_extent] = choose_swapchain_options(capabilities, formats, present_modes, window->get_window());

    device_ = device;
    window_ = window;
}

Swapchain::~Swapchain() {

}

std::tuple<VkSurfaceFormatKHR, VkPresentModeKHR, VkExtent2D> choose_swapchain_options(VkSurfaceCapabilitiesKHR capabilities, const std::vector<VkSurfaceFormatKHR> &formats, const std::vector<VkPresentModeKHR> &present_modes, GLFWwindow *window) {
    VkSurfaceFormatKHR surface_format {};
    VkPresentModeKHR present_mode {};
    VkExtent2D swap_extent {};
    
    uint32_t format_index = 0;
    for (; format_index < formats.size(); ++format_index) {
	if (formats[format_index].format == VK_FORMAT_B8G8R8A8_SRGB && formats[format_index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	    surface_format = formats[format_index];
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
