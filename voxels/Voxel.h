#pragma once

#include <unordered_set>
#include <variant>
#include <vector>
#include <span>
#include <set>

#include <graphics/GraphicsContext.h>

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
	       std::variant<std::vector<std::byte>, std::shared_ptr<GraphicsModel>> &&data,
	       uint32_t width,
	       uint32_t height,
	       uint32_t depth,
	       State state,
	       Format format,
	       AttributeSet attribute_set
	       );

    std::span<const std::byte> get_cpu_data() const;
    uint32_t get_width() const;
    uint32_t get_height() const;
    uint32_t get_depth() const;

private:
    std::variant<std::vector<std::byte>, std::shared_ptr<GraphicsModel>> data_;
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
