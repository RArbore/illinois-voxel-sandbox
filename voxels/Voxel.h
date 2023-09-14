#pragma once

#include <unordered_set>
#include <vector>
#include <span>
#include <set>

#include <graphics/GPUAllocator.h>
#include <graphics/Command.h>
#include <graphics/RingBuffer.h>

class GraphicsModel;
class ChunkManager;

class VoxelChunk {
public:
    enum class State {
	CPU,
	GPU,
    };

    enum class Format {
	Raw,
    };

    enum class AttributeSet {
	Color,
	ColorNormal,
    };

    VoxelChunk(
	       std::vector<std::byte> &&data,
	       uint32_t width,
	       uint32_t height,
	       uint32_t depth,
	       State state,
	       Format format,
	       AttributeSet attribute_set
	       );

    std::span<const std::byte> get_cpu_data() const;
    std::shared_ptr<GPUVolume> get_gpu_volume() const;
    std::shared_ptr<Semaphore> get_timeline() const;
    uint32_t get_width() const;
    uint32_t get_height() const;
    uint32_t get_depth() const;
    Format get_format() const;
    AttributeSet get_attribute_set() const;
    void queue_gpu_upload(std::shared_ptr<Device> device,
			  std::shared_ptr<GPUAllocator> allocator,
			  std::shared_ptr<RingBuffer> ring_buffer);
    
private:
    std::vector<std::byte> cpu_data_;
    std::shared_ptr<GPUVolume> volume_data_;
    std::shared_ptr<Semaphore> timeline_ = nullptr;
    uint32_t width_;
    uint32_t height_;
    uint32_t depth_;
    State state_;
    Format format_;
    AttributeSet attribute_set_;

    friend class ChunkManager;
};

class VoxelChunkPtr {
public:
    VoxelChunk &operator*();
    VoxelChunk *operator->();
private:
    ChunkManager *manager_;
    uint64_t chunk_idx_;
    friend class ChunkManager;
};

class ChunkManager {
public:
    VoxelChunkPtr add_chunk(std::vector<std::byte> &&data, uint32_t width, uint32_t height, uint32_t depth, VoxelChunk::Format format, VoxelChunk::AttributeSet attribute_set);
private:
    std::vector<VoxelChunk> chunks_;

    friend class VoxelChunk;
    friend class VoxelChunkPtr;
};

// This should really be in procedural/ or similar...
std::vector<std::byte> generate_basic_sphere_chunk(uint32_t width, uint32_t height, uint32_t depth, float radius);
std::vector<std::byte> generate_basic_filled_chunk(uint32_t width, uint32_t height, uint32_t depth);
