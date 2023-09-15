#pragma once

#include <vector>

#include "RawVoxelChunk.h"
#include "VoxelChunk.h"

class VoxelManager {
  public:
    VoxelManager() = default;
    ~VoxelManager();

  private:
    std::vector<VoxelChunk *> chunks_;

};
