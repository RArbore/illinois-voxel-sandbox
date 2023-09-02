#include "RawVoxelChunk.h"

RawVoxelChunk::RawVoxelChunk(const glm::vec3 &position)
    : VoxelChunk(VoxelFormat::Raw, position) {
    voxels_.fill(RawVoxel{.red_ = 0, .green_ = 0, .blue_ = 0, .alpha_ = 0});
}

void RawVoxelChunk::write_voxel(int x, int y, int z, const RawVoxel &voxel) {
    assert(x < 8 && y < 8 && z < 8);

    int linear_index = 64 * z + 8 * y + x;
    voxels_[linear_index] = voxel;
}
