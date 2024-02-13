#pragma once

#include <unordered_set>
#include <filesystem>
#include <atomic>
#include <vector>
#include <list>
#include <span>
#include <set>

#include <external/CTPL/ctpl_stl.h>

#include <graphics/Command.h>
#include <graphics/GPUAllocator.h>
#include <graphics/RingBuffer.h>

class GraphicsModel;
class ChunkManager;

class VoxelChunk {
  public:
    enum class State {
        CPU,
        GPU,
        Disk,
    };

    enum class Format {
        Raw,
        SVO,
	SVDAG,
	DF,
    };

    enum class AttributeSet {
        Color,
        ColorNormal,
        Emissive,
    };

    VoxelChunk(std::vector<std::byte> &&data, uint32_t width, uint32_t height,
               uint32_t depth, State state, Format format,
               AttributeSet attribute_set, ChunkManager *manager);

    VoxelChunk(std::vector<std::byte> &&data, uint32_t width, uint32_t height,
               uint32_t depth, State state, std::string custom_format,
               ChunkManager *manager);

    std::span<const std::byte> get_cpu_data() const;
    std::shared_ptr<GPUBuffer> get_gpu_buffer() const;
    std::shared_ptr<Semaphore> get_timeline() const;
    uint32_t get_width() const;
    uint32_t get_height() const;
    uint32_t get_depth() const;
    State get_state() const;
    Format get_format() const;
    AttributeSet get_attribute_set() const;
    const std::string &get_custom_format() const;
    void tick_gpu_upload(std::shared_ptr<Device> device,
                          std::shared_ptr<GPUAllocator> allocator,
                          std::shared_ptr<RingBuffer> ring_buffer);
    void tick_disk_download(std::shared_ptr<Device> device,
                            std::shared_ptr<RingBuffer> ring_buffer);
    bool uploading();
    bool downloading();

    void debug_print();

  private:
    std::filesystem::path disk_path_;
    std::vector<std::byte> cpu_data_;
    std::shared_ptr<GPUBuffer> buffer_data_;
    std::shared_ptr<Semaphore> timeline_ = nullptr;
    uint32_t width_;
    uint32_t height_;
    uint32_t depth_;
    std::atomic<State> state_;
    Format format_;
    AttributeSet attribute_set_;
    std::string custom_format_;
    std::atomic_bool uploading_ = false;
    std::atomic_bool downloading_ = false;
    ChunkManager *manager_;

    friend class ChunkManager;
};

class VoxelChunkPtr {
  public:
    VoxelChunk &operator*();
    VoxelChunk *operator->();

  private:
    std::list<VoxelChunk>::iterator it_;
    friend class ChunkManager;
};

class ChunkManager {
  public:
    ChunkManager();
    ~ChunkManager();

    VoxelChunkPtr add_chunk(std::vector<std::byte> &&data, uint32_t width,
                            uint32_t height, uint32_t depth,
                            VoxelChunk::Format format,
                            VoxelChunk::AttributeSet attribute_set);

    VoxelChunkPtr add_chunk(std::vector<std::byte> &&data, uint32_t width,
                            uint32_t height, uint32_t depth,
                            std::string custom_format);

    void debug_print();

  private:
    std::filesystem::path chunks_directory_;
    std::list<VoxelChunk> chunks_;
    ctpl::thread_pool pool_;

    friend class VoxelChunk;
    friend class VoxelChunkPtr;
};

const static std::unordered_map<VoxelChunk::State, std::string_view> state_names {
    {VoxelChunk::State::CPU, "CPU"},
    {VoxelChunk::State::GPU, "GPU"},
    {VoxelChunk::State::Disk, "Disk"},
};

const static std::unordered_map<VoxelChunk::Format, std::string_view> format_names {
    {VoxelChunk::Format::Raw, "Raw"},
    {VoxelChunk::Format::SVO, "SVO"},
    {VoxelChunk::Format::SVDAG, "SVDAG"},
};

const static std::unordered_map<VoxelChunk::AttributeSet, std::string_view> attribute_set_names {
    {VoxelChunk::AttributeSet::Color, "Color"},
    {VoxelChunk::AttributeSet::ColorNormal, "ColorNormal"},
    {VoxelChunk::AttributeSet::Emissive, "Emissive"}
};
