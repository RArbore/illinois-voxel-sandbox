#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "VoxelChunk.h"

class VoxelManager {
public:
	VoxelManager() = default;

	VoxelChunk& generate_test_cube(const glm::vec3& position);

private:
	std::vector<VoxelChunk> chunks_;
};