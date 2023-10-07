#include <cmath>

#include "Voxel.h"
#include "utils/Assert.h"

VoxelChunk::VoxelChunk(std::vector<std::byte> &&data, uint32_t width,
                       uint32_t height, uint32_t depth, State state,
                       Format format, AttributeSet attribute_set)
    : cpu_data_(data), width_(width), height_(height), depth_(depth),
      state_(state), format_(format), attribute_set_(attribute_set) {}

std::span<const std::byte> VoxelChunk::get_cpu_data() const {
    ASSERT(state_ == State::CPU,
           "Tried to get CPU data of voxel chunk without CPU data resident.");
    return std::span{cpu_data_};
}

std::shared_ptr<GPUBuffer> VoxelChunk::get_gpu_buffer() const {
    ASSERT(state_ == State::GPU, "Tried to get GPU buffer data of voxel chunk "
                                 "without GPU data resident.");
    return buffer_data_;
}

std::shared_ptr<GPUVolume> VoxelChunk::get_gpu_volume() const {
    ASSERT(state_ == State::GPU, "Tried to get GPU volume data of voxel chunk "
                                 "without GPU data resident.");
    return volume_data_;
}

std::shared_ptr<Semaphore> VoxelChunk::get_timeline() const {
    return timeline_;
}

uint32_t VoxelChunk::get_width() const { return width_; }

uint32_t VoxelChunk::get_height() const { return height_; }

uint32_t VoxelChunk::get_depth() const { return depth_; }

VoxelChunk::State VoxelChunk::get_state() const { return state_; }

VoxelChunk::Format VoxelChunk::get_format() const { return format_; }

VoxelChunk::AttributeSet VoxelChunk::get_attribute_set() const {
    return attribute_set_;
}

void VoxelChunk::queue_gpu_upload(std::shared_ptr<Device> device,
                                  std::shared_ptr<GPUAllocator> allocator,
                                  std::shared_ptr<RingBuffer> ring_buffer) {
    ASSERT(state_ == State::CPU, "Tried to upload CPU data of voxel chunk to "
                                 "GPU without CPU data resident.");
    std::cout << "INFO: Queued upload to GPU for voxel chunk " << this
              << ", which has the following dimensions: (" << width_ << ", "
              << height_ << ", " << depth_ << ").\n";
    if (timeline_ == nullptr) {
        timeline_ = std::make_shared<Semaphore>(device, true);
        timeline_->set_signal_value(1);
    }
    switch (format_) {
    case Format::Raw: {
        VkExtent3D extent = {width_, height_, depth_};
        volume_data_ = std::make_shared<GPUVolume>(
            allocator, extent, VK_FORMAT_R8G8B8A8_UNORM, 0,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 1, 1);
        ring_buffer->copy_to_device(volume_data_, VK_IMAGE_LAYOUT_GENERAL,
                                    get_cpu_data(), {timeline_}, {timeline_});
        break;
    }
    case Format::SVO: {
        buffer_data_ =
            std::make_shared<GPUBuffer>(allocator, get_cpu_data().size(), 8,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
        ring_buffer->copy_to_device(buffer_data_, 0, get_cpu_data(),
                                    {timeline_}, {timeline_});
        break;
    }
    default: {
        ASSERT(false, "GPU upload for format is unimplemented.");
    }
    }
    state_ = State::GPU;
    timeline_->increment();
}

VoxelChunk &VoxelChunkPtr::operator*() {
    return manager_->chunks_.at(chunk_idx_);
}

VoxelChunk *VoxelChunkPtr::operator->() {
    return &manager_->chunks_.at(chunk_idx_);
}

ChunkManager::ChunkManager() {
    std::stringstream string_pointer;
    string_pointer << this;
    chunks_directory_ = "disk_chunks_" + string_pointer.str();
    std::filesystem::create_directory(chunks_directory_);
}

ChunkManager::~ChunkManager() {
    std::filesystem::remove_all(chunks_directory_);
}

VoxelChunkPtr ChunkManager::add_chunk(std::vector<std::byte> &&data,
                                      uint32_t width, uint32_t height,
                                      uint32_t depth, VoxelChunk::Format format,
                                      VoxelChunk::AttributeSet attribute_set) {
    VoxelChunkPtr ptr;
    ptr.manager_ = this;
    ptr.chunk_idx_ = chunks_.size();
    chunks_.emplace_back(std::move(data), width, height, depth,
                         VoxelChunk::State::CPU, format, attribute_set);
    return ptr;
}
