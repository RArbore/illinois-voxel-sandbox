#pragma once

#include <memory>
#include <string>

#include "GPUAllocator.h"
#include "Command.h"
#include "Synchronization.h"

// The functionality here can maybe placed in the GraphicsContext
void copy_buffer_to_image(std::shared_ptr<CommandPool> command_pool,
                          std::shared_ptr<GPUBuffer> buffer,
                          std::shared_ptr<GPUImage> image, 
                          size_t width, size_t height);

std::shared_ptr<GPUImage> load_image(std::shared_ptr<GPUAllocator> allocator,
                                     std::shared_ptr<CommandPool> command_pool,
                                     const std::string &image_name);