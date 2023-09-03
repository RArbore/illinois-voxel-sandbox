#pragma once

#include <vector>

#include "VoxelChunk.h"

struct RawVoxel {
    uint8_t red_;
    uint8_t green_;
    uint8_t blue_;
    uint8_t alpha_;
};

class RawVoxelChunk : public VoxelChunk {
  public:
    RawVoxelChunk(const glm::vec3 &position, int w, int h, int d);

    void write_voxel(int x, int y, int z, const RawVoxel &voxel);

    std::vector<RawVoxel> voxels_;
    int w_, h_, d_;
};
