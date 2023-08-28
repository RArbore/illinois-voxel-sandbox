#include "RawVoxelChunk.h"

RawVoxelChunk::RawVoxelChunk(const glm::vec3& position) :
	VoxelChunk(VoxelFormat::Raw, position)
{
	memset(voxels_, 0, 64);
}

void RawVoxelChunk::write_voxel(uint8_t x, uint8_t y, uint8_t z) {
	assert(x < 8 && y < 8 && z < 8);

	// Each char (1 byte) is a row of the chunk,
	// so a 2d z-slice is a continuous array of 8 chars.
	size_t voxel_index = 8 * z + y;
	voxels_[voxel_index] |= 1 << x;

	// Also write out to the attribute list
	int attribute_index = 64 * z + 8 * y + x;
	attributes_[attribute_index] = RawVoxel{ 
		.color_ = glm::vec3(0.0f, 0.0f, 0.0f) 
	};
}