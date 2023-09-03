#include "RawVoxelChunk.h"

RawVoxelChunk::RawVoxelChunk(const glm::vec3 &position, int w, int h, int d)
    : VoxelChunk(VoxelFormat::Raw, position),
      voxels_(w * h * d, RawVoxel{.red_ = 0, .green_ = 0, .blue_ = 0, .alpha_ = 0}), w_(w), h_(h), d_(d)
{}

void RawVoxelChunk::write_voxel(int x, int y, int z, const RawVoxel &voxel) {
    assert(x < w_ && y < h_ && z < d_);

    int linear_index = w_ * h_ * z + w_ * y + x;
    voxels_[linear_index] = voxel;
}
