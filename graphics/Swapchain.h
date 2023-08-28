#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Window.h"
#include "Descriptor.h"
#include "Synchronization.h"

class Swapchain {
public:
    Swapchain(std::shared_ptr<Device> device, std::shared_ptr<Window> window);
    ~Swapchain();

    std::vector<std::shared_ptr<DescriptorSet>> make_image_descriptors(std::shared_ptr<DescriptorAllocator> allocator);

    VkImage get_image(uint32_t image_index);
    uint32_t acquire_next_image(std::shared_ptr<Semaphore> notify);
    void present_image(uint32_t image_index, std::shared_ptr<Semaphore> wait);
private:
    VkSwapchainKHR swapchain_;
    VkFormat swapchain_format_;
    VkExtent2D swapchain_extent_;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;

    std::shared_ptr<Device> device_;
    std::shared_ptr<Window> window_;
};
