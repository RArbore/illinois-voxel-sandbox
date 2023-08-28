#include "VoxelManager.h"

VoxelManager::~VoxelManager() {
	for (VoxelChunk* chunk : chunks_) {
		delete chunk;
	}
}

VoxelChunk* VoxelManager::generate_test_cube(const glm::vec3& position) {
	RawVoxelChunk* new_chunk = new RawVoxelChunk(position);

	// The corners will be written to multiple times, but this shouldn't affect anything
	for (int z = 0; z < 8; z++) {
		new_chunk->write_voxel(0, 0, z);
		new_chunk->write_voxel(0, 7, z);
		new_chunk->write_voxel(7, 0, z);
		new_chunk->write_voxel(7, 7, z);
	}

	for (int y = 0; y < 8; y++) {
		new_chunk->write_voxel(0, y, 0);
		new_chunk->write_voxel(0, y, 7);
		new_chunk->write_voxel(7, y, 0);
		new_chunk->write_voxel(7, y, 7);
	}

	for (int x = 0; x < 8; x++) {
		new_chunk->write_voxel(x, 0, 0);
		new_chunk->write_voxel(x, 0, 7);
		new_chunk->write_voxel(x, 7, 0);
		new_chunk->write_voxel(x, 7, 7);
	}

	chunks_.push_back(new_chunk);
	return new_chunk;
}