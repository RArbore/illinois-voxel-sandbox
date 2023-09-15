#include "VoxelManager.h"
#include "PerlinNoise.hpp"
#include <cmath>


VoxelManager::~VoxelManager() {
    for (VoxelChunk *chunk : chunks_) {
        delete chunk;
    }
}

