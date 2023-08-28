#pragma once

#include <map>

#include <glm/glm.hpp>

#include "VoxelChunk.h"

struct RawVoxel {
	glm::vec3 color_;
};

class RawVoxelChunk : public VoxelChunk {
public:
	RawVoxelChunk(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));

	virtual void write_voxel(uint8_t x, uint8_t y, uint8_t z) override;

private:
	// An unsigned char is 1 byte, and we want 512 bits = 64 bytes
	unsigned char voxels_[64];
	std::map<int, RawVoxel> attributes_;
};