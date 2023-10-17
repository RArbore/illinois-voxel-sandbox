#include "ExternalImage.h"

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb_image.h>

std::shared_ptr<GPUImage> load_image(std::shared_ptr<GPUAllocator> allocator,
				     std::shared_ptr<RingBuffer> ring_buffer,
                                     const std::string &image_name) {
    const std::string asset_directory = ASSET_DIRECTORY;
    std::string image_path = asset_directory + "/" + image_name;

    int width, height, channels;
    stbi_uc *pixels = stbi_load(image_path.c_str(), &width, &height, &channels,
                                STBI_rgb_alpha);

    ASSERT(pixels, "Unable to load texture.");
    size_t image_size = width * height * 4;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent = {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
    VkImageUsageFlags image_flags =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    std::shared_ptr<GPUImage> texture_image = std::make_shared<GPUImage>(
        allocator, extent, format, 0, image_flags, memory_flags, 0, 1, 1);

    auto image_span = std::span<std::byte>(reinterpret_cast<std::byte *>(pixels), image_size);
    ring_buffer->copy_to_device(texture_image, VK_IMAGE_LAYOUT_GENERAL, image_span, {}, {});
    
    stbi_image_free(pixels);

    return texture_image;
}

