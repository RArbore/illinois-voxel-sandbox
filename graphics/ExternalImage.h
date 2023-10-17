#pragma once

#include <memory>
#include <string>

#include "GPUAllocator.h"
#include "RingBuffer.h"
#include "Synchronization.h"

std::shared_ptr<GPUImage> load_image(std::shared_ptr<GPUAllocator> allocator,
				     std::shared_ptr<RingBuffer> ring_buffer,
                                     const std::string &image_name);
