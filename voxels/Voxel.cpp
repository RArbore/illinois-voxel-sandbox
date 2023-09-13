#include <cmath>

#include "Voxel.h"
#include "utils/Assert.h"

VoxelChunk::VoxelChunk(
		       std::vector<std::byte> &&data,
		       uint32_t width,
		       uint32_t height,
		       uint32_t depth,
		       State state,
		       Format format,
		       AttributeSet attribute_set
		       ): cpu_data_(data),
			  width_(width),
			  height_(height),
			  depth_(depth),
			  state_(state),
			  format_(format),
			  attribute_set_(attribute_set) {}

std::span<const std::byte> VoxelChunk::get_cpu_data() const {
    ASSERT(state_ == State::CPU, "PANIC: Tried to get CPU data of voxel chunk without CPU data resident.");
    return std::span {cpu_data_};
}

std::shared_ptr<GPUVolume> VoxelChunk::get_gpu_volume() const {
    ASSERT(state_ == State::GPU, "PANIC: Tried to get CPU data of voxel chunk without CPU data resident.");
    return volume_data_;
}

uint32_t VoxelChunk::get_width() const {
    return width_;
}

uint32_t VoxelChunk::get_height() const {
    return height_;
}

uint32_t VoxelChunk::get_depth() const {
    return depth_;
}

VoxelChunk::Format VoxelChunk::get_format() const {
    return format_;
}

VoxelChunk::AttributeSet VoxelChunk::get_attribute_set() const {
    return attribute_set_;
}

void VoxelChunk::queue_gpu_upload(std::shared_ptr<Device> device,
				  std::shared_ptr<GPUAllocator> allocator,
				  std::shared_ptr<RingBuffer> ring_buffer) {
    ASSERT(state_ == State::CPU, "PANIC: Tried to upload CPU data of voxel chunk to GPU without CPU data resident.");
    if (timeline_ == nullptr) {
	timeline_ = std::make_shared<Semaphore>(device, true);
	timeline_->set_signal_value(1);
    }
    switch (format_) {
    case Format::Raw: {
	VkExtent3D extent = {width_, height_,
			     depth_};
	volume_data_ = std::make_shared<GPUVolume>(
						   allocator, extent, VK_FORMAT_R8G8B8A8_UNORM, 0,
						   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 1, 1);
	ring_buffer->copy_to_device(volume_data_, VK_IMAGE_LAYOUT_GENERAL, get_cpu_data(), {timeline_}, {timeline_});
	break;
    }
    default: {
	ASSERT(false, "PANIC: GPU upload for format is unimplemented.");
    }
    }
    state_ = State::GPU;
}

VoxelChunk &VoxelChunkPtr::operator*() {
    return manager_->chunks_.at(chunk_idx_);
}

VoxelChunk *VoxelChunkPtr::operator->() {
    return &manager_->chunks_.at(chunk_idx_);
}

VoxelChunkPtr ChunkManager::add_chunk(std::vector<std::byte> &&data, uint32_t width, uint32_t height, uint32_t depth, VoxelChunk::Format format, VoxelChunk::AttributeSet attribute_set) {
    VoxelChunkPtr ptr;
    ptr.manager_ = this;
    ptr.chunk_idx_ = chunks_.size();
    chunks_.emplace_back(std::move(data), width, height, depth, VoxelChunk::State::CPU, format, attribute_set);
    return ptr;
}

std::vector<std::byte> generate_basic_sphere_chunk(uint32_t width, uint32_t height, uint32_t depth, float radius) {
    std::vector<std::byte> data(width * height * depth * 4, static_cast<std::byte>(0));
    
    for (uint32_t x = 0; x < width; ++x) {
	for (uint32_t y = 0; y < height; ++y) {
	    for (uint32_t z = 0; z < depth; ++z) {
                if (std::pow(static_cast<double>(x) - width / 2, 2) + std::pow(static_cast<double>(y) - height / 2, 2) + std::pow(static_cast<double>(z) - depth / 2, 2) < std::pow(radius, 2)) {
		    std::byte red = static_cast<std::byte>(x * 4);
		    std::byte green = static_cast<std::byte>(y * 4);
		    std::byte blue = static_cast<std::byte>(z * 4);
		    std::byte alpha = static_cast<std::byte>(255);
		    
		    size_t voxel_idx = x + y * width + z * width * height;
		    data.at(voxel_idx * 4) = red;
		    data.at(voxel_idx * 4 + 1) = green;
		    data.at(voxel_idx * 4 + 2) = blue;
		    data.at(voxel_idx * 4 + 3) = alpha;
		}
	    }
	}
    }

    return data;
}

std::vector<std::byte> generate_basic_filled_chunk(uint32_t width, uint32_t height, uint32_t depth) {
    std::vector<std::byte> data(width * height * depth * 4, static_cast<std::byte>(0));
    
    for (uint32_t x = 0; x < width; ++x) {
	for (uint32_t y = 0; y < height; ++y) {
	    for (uint32_t z = 0; z < depth; ++z) {
		std::byte red = static_cast<std::byte>(x * 30);
		std::byte green = static_cast<std::byte>(y * 30);
		std::byte blue = static_cast<std::byte>(z * 30);
		std::byte alpha = static_cast<std::byte>(255);
		
		size_t voxel_idx = x + y * width + z * width * height;
		data.at(voxel_idx * 4) = red;
		data.at(voxel_idx * 4 + 1) = green;
		data.at(voxel_idx * 4 + 2) = blue;
		data.at(voxel_idx * 4 + 3) = alpha;
	    }
	}
    }

    return data;
}
