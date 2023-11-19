#include <filesystem>
#include <fstream>
#include <cmath>

#include "Voxel.h"
#include "utils/Assert.h"

VoxelChunk::VoxelChunk(std::vector<std::byte> &&data, uint32_t width,
                       uint32_t height, uint32_t depth, State state,
                       Format format, AttributeSet attribute_set, ChunkManager *manager)
    : cpu_data_(data), width_(width), height_(height), depth_(depth),
      state_(state), format_(format), attribute_set_(attribute_set), manager_(manager) {}

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

void VoxelChunk::tick_gpu_upload(std::shared_ptr<Device> device,
                                  std::shared_ptr<GPUAllocator> allocator,
                                  std::shared_ptr<RingBuffer> ring_buffer) {
    if (state_ == State::Disk && !uploading_ && !downloading_) {
	uploading_ = true;
	manager_->pool_.push([this](int _id){
            std::ifstream stream(disk_path_,
                                  std::ios::in | std::ios::binary);
            const auto size = std::filesystem::file_size(disk_path_);
	    cpu_data_ = std::vector<std::byte>(size);
	    stream.read(reinterpret_cast<char*>(cpu_data_.data()), size);
	    std::filesystem::remove(disk_path_);

	    state_ = State::CPU;
	});
    } else if (state_ == State::CPU) {
	uploading_ = true;
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
	case Format::SVO:
	case Format::SVDAG: {
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
    } else if (state_ == State::GPU && timeline_->has_reached_wait()) {
	uploading_ = false;
    }
}

void VoxelChunk::tick_disk_download(std::shared_ptr<Device> device,
                                    std::shared_ptr<RingBuffer> ring_buffer) {
    if (!downloading_ && state_ != State::Disk) {
	buffer_data_ = nullptr;
	volume_data_ = nullptr;
	state_ = State::CPU;
	downloading_ = true;
	manager_->pool_.push([this](int _id){
	    std::stringstream string_pointer;
	    string_pointer << this;
	    if (disk_path_.empty()) {
		disk_path_ = manager_->chunks_directory_ / std::filesystem::path("chunk_" + string_pointer.str());
	    }
	    std::ofstream stream(disk_path_,
				 std::ios::out | std::ios::binary);
	    stream.write(reinterpret_cast<char*>(cpu_data_.data()), cpu_data_.size());
	    cpu_data_.clear();
	    
	    state_ = State::Disk;
	    downloading_ = false;
	});
    }
}

bool VoxelChunk::uploading() {
    return uploading_;
}
	
bool VoxelChunk::downloading() {
    return downloading_;
}

void VoxelChunk::debug_print() {
    std::cout << "DEBUG PRINT FOR CHUNK " << this << "\n";
    std::cout << "  disk_path_: " << disk_path_ << "\n";
    std::cout << "  cpu_data_: vector with size " << cpu_data_.size() << "\n";
    if (buffer_data_) {
	std::cout << "  buffer_data_: GPUBuffer with size " << buffer_data_->get_size() << "\n";
    } else {
	std::cout << "  buffer_data_: (null)\n";
    }
    if (volume_data_) {
	std::cout << "  volume_data_: GPUVolume with dimensions (" << volume_data_->get_extent().width << ", " << volume_data_->get_extent().height << ", " << volume_data_->get_extent().depth << ")\n";
    } else {
	std::cout << "  volume_data_: (null)\n";
    }
    std::cout << "  timeline_: " << timeline_ << "\n";
    std::cout << "  width_: " << width_ << "\n";
    std::cout << "  height_: " << height_ << "\n";
    std::cout << "  depth_: " << depth_ << "\n";
    std::cout << "  state_: " << state_names.at(state_) << "\n";
    std::cout << "  format_: " << format_names.at(format_) << "\n";
    std::cout << "  attribute_: set_" << attribute_set_names.at(attribute_set_) << "\n";
    std::cout << "  uploading_: " << uploading_ << "\n";
    std::cout << "  downloading_: " << downloading_ << "\n";
    std::cout << "  manager_: " << manager_ << "\n";
    std::cout << "\n";
}

VoxelChunk &VoxelChunkPtr::operator*() {
    return *it_;
}

VoxelChunk *VoxelChunkPtr::operator->() {
    return &*it_;
}

ChunkManager::ChunkManager(): pool_(8) {
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
    chunks_.emplace_front(std::move(data), width, height, depth,
			  VoxelChunk::State::CPU, format, attribute_set, this);
    ptr.it_ = chunks_.begin();
    return ptr;
}

void ChunkManager::debug_print() {
    for (auto &chunk : chunks_) {
	chunk.debug_print();
    }
}
