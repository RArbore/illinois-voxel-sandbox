#pragma once

#include <external/glm/glm/glm.hpp>

enum class VoxelFormat {
    Raw = 0,
};

class VoxelChunk {
public:
    VoxelChunk() = delete;
    VoxelChunk(VoxelFormat format, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));

    // Probably needs a better definition at some point
    virtual void write_voxel(uint8_t x, uint8_t y, uint8_t z) = 0;

    VoxelFormat get_chunk_format() const { return format_; };

private:
    glm::vec3 position_;
    VoxelFormat format_;
    
    size_t calculate_size(int w, int d, int h);
};
