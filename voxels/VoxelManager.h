#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "VoxelChunk.h"
#include "RawVoxelChunk.h"

class VoxelManager {
public:
	VoxelManager() = default;
	~VoxelManager();

	VoxelChunk* generate_test_cube(const glm::vec3& position);

private:
	std::vector<VoxelChunk*> chunks_;
};