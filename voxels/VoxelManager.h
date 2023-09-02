#pragma once

#include <vector>

#include "RawVoxelChunk.h"
#include "VoxelChunk.h"

class VoxelManager {
  public:
    VoxelManager() = default;
    ~VoxelManager();

    RawVoxelChunk *generate_test_cube(const glm::vec3 &position);

  private:
    std::vector<VoxelChunk *> chunks_;
};
