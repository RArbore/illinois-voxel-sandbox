#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Window.h"
#include "Device.h"

class Swapchain {
public:
    Swapchain(std::shared_ptr<Device> device, std::shared_ptr<Window> window);
    ~Swapchain();
private:
    VkSwapchainKHR swapchain_;
    VkFormat swapchain_format_;
    VkExtent2D swapchain_extent_;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;

    std::shared_ptr<Device> device_;
    std::shared_ptr<Window> window_;
};
