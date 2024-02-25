#pragma once

#include "Voxel.h"

std::vector<std::byte> raw_voxelize_obj(std::string_view filepath,
                                        float voxel_size,
                                        uint32_t &out_chunk_width,
                                        uint32_t &out_chunk_height,
                                        uint32_t &out_chunk_depth);

class Voxelizer {
  private:
    uint32_t width_, height_, depth_;
    std::vector<std::byte> voxels_;

  public:
    Voxelizer(std::vector<std::byte> &&voxels, uint32_t width, uint32_t height,
              uint32_t depth)
        : width_(width), height_(height), depth_(depth), voxels_(voxels) {}
    uint32_t at(uint32_t x, uint32_t y, uint32_t z);
};
