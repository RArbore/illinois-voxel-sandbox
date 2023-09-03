#include "VoxelManager.h"

VoxelManager::~VoxelManager() {
    for (VoxelChunk *chunk : chunks_) {
        delete chunk;
    }
}

RawVoxelChunk *VoxelManager::generate_test_cube(const glm::vec3 &position) {
    RawVoxelChunk *new_chunk = new RawVoxelChunk(position, 11, 11, 11);
    RawVoxel voxel =
        RawVoxel{.red_ = 255, .green_ = 0, .blue_ = 255, .alpha_ = 255};

    // The corners will be written to multiple times, but this shouldn't affect
    // anything
    for (int z = 0; z < 11; z++) {
        new_chunk->write_voxel(0, 0, z, voxel);
        new_chunk->write_voxel(0, 10, z, voxel);
        new_chunk->write_voxel(10, 0, z, voxel);
        new_chunk->write_voxel(10, 10, z, voxel);
    }

    voxel = RawVoxel{.red_ = 0, .green_ = 255, .blue_ = 255, .alpha_ = 255};
    for (int y = 0; y < 11; y++) {
        new_chunk->write_voxel(0, y, 0, voxel);
        new_chunk->write_voxel(0, y, 10, voxel);
        new_chunk->write_voxel(10, y, 0, voxel);
        new_chunk->write_voxel(10, y, 10, voxel);
    }

    voxel = RawVoxel{.red_ = 255, .green_ = 255, .blue_ = 0, .alpha_ = 255};
    for (int x = 0; x < 11; x++) {
        new_chunk->write_voxel(x, 0, 0, voxel);
        new_chunk->write_voxel(x, 0, 10, voxel);
        new_chunk->write_voxel(x, 10, 0, voxel);
        new_chunk->write_voxel(x, 10, 10, voxel);
    }

    chunks_.push_back(new_chunk);

    return new_chunk;
}
