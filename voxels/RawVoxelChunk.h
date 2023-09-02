#pragma once

#include <array>

#include "VoxelChunk.h"

struct RawVoxel {
    uint8_t red_;
    uint8_t green_;
    uint8_t blue_;
    uint8_t alpha_;
};

class RawVoxelChunk : public VoxelChunk {
  public:
    RawVoxelChunk(const glm::vec3 &position);

    void write_voxel(int x, int y, int z, const RawVoxel &voxel);

    // private:
    std::array<RawVoxel, 512> voxels_;
};
