#pragma once

#include <glm/glm.hpp>

enum class VoxelFormat {
    Raw,
    Brickmap,
};

class VoxelChunk {
public:
    VoxelChunk() = delete;
    VoxelChunk(VoxelFormat format, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));

    VoxelFormat get_chunk_format() const { return format_; };

private:
    glm::vec3 position_;
    VoxelFormat format_;
    
    size_t calculate_size(int w, int d, int h);
};
