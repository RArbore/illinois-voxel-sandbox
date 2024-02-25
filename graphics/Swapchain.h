#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Descriptor.h"
#include "Synchronization.h"
#include "Window.h"

class Swapchain {
  public:
    Swapchain(std::shared_ptr<Device> device, std::shared_ptr<Window> window,
              std::shared_ptr<DescriptorAllocator> allocator);
    ~Swapchain();

    const std::vector<VkImageView> &get_image_views() const;
    VkImage get_image(uint32_t image_index);
    VkExtent2D get_extent();
    VkFormat get_format();
    uint32_t acquire_next_image(std::shared_ptr<Semaphore> notify);
    void present_image(uint32_t image_index, std::shared_ptr<Semaphore> wait);
    std::shared_ptr<DescriptorSet> get_image_descriptor(uint32_t image_index);

  private:
    void make_image_descriptors(std::shared_ptr<DescriptorAllocator> allocator);
    void recreate();

    VkSwapchainKHR swapchain_;
    VkFormat swapchain_format_;
    VkExtent2D swapchain_extent_;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    std::vector<std::shared_ptr<DescriptorSet>> descriptors_;

    std::shared_ptr<Device> device_;
    std::shared_ptr<Window> window_;
    std::shared_ptr<DescriptorAllocator> allocator_;
};
