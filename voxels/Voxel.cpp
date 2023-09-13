#include <cmath>

#include "Voxel.h"
#include "utils/Assert.h"

VoxelChunk::VoxelChunk(
		       std::variant<std::vector<std::byte>, std::shared_ptr<GPUVolume>> &&data,
		       uint32_t width,
		       uint32_t height,
		       uint32_t depth,
		       State state,
		       Format format,
		       AttributeSet attribute_set
		       ): data_(data),
			  width_(width),
			  height_(height),
			  depth_(depth),
			  state_(state),
			  format_(format),
			  attribute_set_(attribute_set) {}

std::span<const std::byte> VoxelChunk::get_cpu_data() const {
    ASSERT(format_ == Format::Raw, "PANIC: Tried to get CPU data of voxel chunk without CPU data resident.");
    return std::span {std::get<0>(data_)};
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
    chunks_.emplace_back(data, width, height, depth, VoxelChunk::State::CPU, format, attribute_set);
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
