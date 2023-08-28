#pragma once

#include <glm/glm.hpp>

class VoxelChunk {
    public:
        VoxelChunk CreateChunk();
        VoxelChunk CreateChunk(float x, float y, float z, int w, int d, int h);
        VoxelChunk CreateChunk(glm::vec3 pos, glm::vec3 dim);
        enum Type { basic = 0, Int = 1, Float = 2, Vec3 = 3 };
    private:
        glm::vec3 position_;
        std::shared_ptr<void> chunk_; // Not sure if we need shared_ptrs here if each chunk manages its own set of voxels
        uint16_t CalculateSize(int w, int d, int h, Type t);
};
