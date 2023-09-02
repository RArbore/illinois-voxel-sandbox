#include "GPUAllocator.h"
#include "utils/Assert.h"

GPUAllocator::GPUAllocator(std::shared_ptr<Device> device) : device_(device) {
    VmaAllocatorCreateInfo create_info{};
    create_info.instance = device->get_instance();
    create_info.physicalDevice = device->get_physical_device();
    create_info.device = device->get_device();
    create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    ASSERT(vmaCreateAllocator(&create_info, &allocator_),
           "Couldn't create allocator.");
}

GPUAllocator::~GPUAllocator() { vmaDestroyAllocator(allocator_); }

VmaAllocator GPUAllocator::get_vma() { return allocator_; }

std::shared_ptr<Device> GPUAllocator::get_device() { return device_; }

GPUBuffer::GPUBuffer(std::shared_ptr<GPUAllocator> allocator, VkDeviceSize size,
                     VkDeviceSize alignment, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags memory_flags,
                     VmaAllocationCreateFlags vma_flags) {
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.flags = vma_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.requiredFlags = memory_flags;

    allocator_ = allocator;
    if (size > 0) {
        ASSERT(vmaCreateBufferWithAlignment(allocator_->get_vma(), &create_info,
                                            &alloc_info, alignment, &buffer_,
                                            &allocation_, nullptr),
               "Unable to create buffer.");
    }

    size_ = size;
    usage_ = usage;
    memory_flags_ = memory_flags;
    vma_flags_ = vma_flags;
}

GPUBuffer::~GPUBuffer() {
    vmaDestroyBuffer(allocator_->get_vma(), buffer_, allocation_);
}

VkBuffer GPUBuffer::get_buffer() { return buffer_; }

VkDeviceAddress GPUBuffer::get_device_address() {
    VkBufferDeviceAddressInfo buffer_device_address_info{};
    buffer_device_address_info.sType =
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buffer_device_address_info.buffer = buffer_;
    return vkGetBufferDeviceAddress(allocator_->get_device()->get_device(),
                                    &buffer_device_address_info);
}

VkDeviceSize GPUBuffer::get_size() { return size_; }

std::shared_ptr<GPUAllocator> GPUBuffer::get_allocator() { return allocator_; }

std::span<std::byte> GPUBuffer::cpu_map() {
    void *data;
    vmaMapMemory(allocator_->get_vma(), allocation_, &data);
    return std::span<std::byte>(reinterpret_cast<std::byte *>(data), size_);
}

void GPUBuffer::cpu_unmap() {
    vmaUnmapMemory(allocator_->get_vma(), allocation_);
}

GPUImage::GPUImage(std::shared_ptr<GPUAllocator> allocator, VkExtent2D extent,
                   VkFormat format, VkImageCreateFlags create_flags,
                   VkImageUsageFlags usage_flags,
                   VkMemoryPropertyFlags memory_flags,
                   VmaAllocationCreateFlags vma_flags, uint32_t mip_levels,
                   uint32_t array_layers) {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = create_flags;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = extent.width;
    image_create_info.extent.height = extent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = mip_levels;
    image_create_info.arrayLayers = array_layers;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage_flags;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.flags = vma_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.requiredFlags = memory_flags;

    allocator_ = allocator;
    ASSERT(vmaCreateImage(allocator_->get_vma(), &image_create_info,
                          &alloc_info, &image_, &allocation_, nullptr),
           "Unable to create image.");
    extent_ = extent;
    format_ = format;
    create_flags_ = create_flags;
    usage_flags_ = usage_flags;
    memory_flags_ = memory_flags;
    vma_flags_ = vma_flags;
    mip_levels_ = mip_levels;
    array_layers_ = array_layers;
    last_set_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image_;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format_;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = mip_levels_;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = array_layers_;

    ASSERT(vkCreateImageView(allocator_->get_device()->get_device(),
                             &image_view_create_info, nullptr, &view_),
           "Unable to create image view.");
}

GPUImage::~GPUImage() {
    vkDestroyImageView(allocator_->get_device()->get_device(), view_, nullptr);
    vmaDestroyImage(allocator_->get_vma(), image_, allocation_);
}

VkImage GPUImage::get_image() { return image_; }

VkImageView GPUImage::get_view() { return view_; }

VkExtent2D GPUImage::get_extent() { return extent_; }

GPUVolume::GPUVolume(std::shared_ptr<GPUAllocator> allocator, VkExtent3D extent,
                     VkFormat format, VkImageCreateFlags create_flags,
                     VkImageUsageFlags usage_flags,
                     VkMemoryPropertyFlags memory_flags,
                     VmaAllocationCreateFlags vma_flags, uint32_t mip_levels,
                     uint32_t array_layers) {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.flags = create_flags;
    image_create_info.imageType = VK_IMAGE_TYPE_3D;
    image_create_info.format = format;
    image_create_info.extent.width = extent.width;
    image_create_info.extent.height = extent.height;
    image_create_info.extent.depth = extent.depth;
    image_create_info.mipLevels = mip_levels;
    image_create_info.arrayLayers = array_layers;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage_flags;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.flags = vma_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.requiredFlags = memory_flags;

    allocator_ = allocator;
    ASSERT(vmaCreateImage(allocator_->get_vma(), &image_create_info,
                          &alloc_info, &image_, &allocation_, nullptr),
           "Unable to create image.");
    extent_ = extent;
    format_ = format;
    create_flags_ = create_flags;
    usage_flags_ = usage_flags;
    memory_flags_ = memory_flags;
    vma_flags_ = vma_flags;
    mip_levels_ = mip_levels;
    array_layers_ = array_layers;
    last_set_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image_;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
    image_view_create_info.format = format_;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = mip_levels_;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = array_layers_;

    ASSERT(vkCreateImageView(allocator_->get_device()->get_device(),
                             &image_view_create_info, nullptr, &view_),
           "Unable to create image view.");
}

GPUVolume::~GPUVolume() {
    vkDestroyImageView(allocator_->get_device()->get_device(), view_, nullptr);
    vmaDestroyImage(allocator_->get_vma(), image_, allocation_);
}

VkImage GPUVolume::get_image() { return image_; }

VkImageView GPUVolume::get_view() { return view_; }

VkExtent3D GPUVolume::get_extent() { return extent_; }
