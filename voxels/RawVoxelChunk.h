#pragma once

#include <array>

#include <glm/glm.hpp>

#include "VoxelChunk.h"

struct RawVoxel {
	glm::vec3 color_;
	bool visible_;
};

class RawVoxelChunk : public VoxelChunk {
public:
	RawVoxelChunk(const glm::vec3& position);

	void write_voxel(int x, int y, int z, const RawVoxel& voxel);

private:
	std::array<RawVoxel, 512> voxels_;
};