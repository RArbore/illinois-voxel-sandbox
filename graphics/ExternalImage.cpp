#include "ExternalImage.h"

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb_image.h>

void copy_buffer_to_image(std::shared_ptr<CommandPool> command_pool,
    std::shared_ptr<GPUBuffer> buffer,
    std::shared_ptr<GPUImage> image, size_t width,
    size_t height) {
    std::shared_ptr<Device> device = command_pool->get_device();
    std::shared_ptr<Command> copy_command_buffer = std::make_shared<Command>(command_pool);

    Barrier copy_barrier{device,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_ACCESS_TRANSFER_WRITE_BIT,
                         image->get_image(),
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(width),
                          static_cast<uint32_t>(height), 1};

    Barrier prep_barrier{device,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_ACCESS_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_ACCESS_SHADER_READ_BIT,
                         image->get_image(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    copy_command_buffer->record([&](VkCommandBuffer command_buffer) {
        copy_barrier.record(command_buffer);

        vkCmdCopyBufferToImage(command_buffer, buffer->get_buffer(), image->get_image(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        prep_barrier.record(command_buffer);
    });

    std::shared_ptr<Fence> fence = std::make_shared<Fence>(device, false);
    device->submit_command(copy_command_buffer, {}, {}, fence);
    fence->wait();
}

std::shared_ptr<GPUImage> load_image(std::shared_ptr<GPUAllocator> allocator,
                                     std::shared_ptr<CommandPool> command_pool,
                                     const std::string &image_name) {
    const std::string asset_directory = ASSET_DIRECTORY;
    std::string image_path = asset_directory + "/" + image_name;

    int width, height, channels;
    stbi_uc *pixels = stbi_load(image_path.c_str(), &width, &height, &channels,
                                STBI_rgb_alpha);

    ASSERT(pixels, "Unable to load texture.");
    size_t image_size = width * height * 4;

    std::shared_ptr<GPUBuffer> staging_buffer = std::make_shared<GPUBuffer>(
        allocator, image_size, 0, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    auto data = staging_buffer->cpu_map();
    void *image_data = reinterpret_cast<void *>(data.data());
    memcpy(image_data, pixels, image_size);
    staging_buffer->cpu_unmap();

    stbi_image_free(pixels);

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent = {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
    VkImageUsageFlags image_flags =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    std::shared_ptr<GPUImage> texture_image = std::make_shared<GPUImage>(
        allocator, extent, format, 0, image_flags, memory_flags, 0, 1, 1);

    copy_buffer_to_image(command_pool, staging_buffer, texture_image, width, height);

    return texture_image;
}

