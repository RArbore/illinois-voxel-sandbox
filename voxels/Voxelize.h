#pragma once

#include "Voxel.h"

std::vector<std::byte> raw_voxelize_obj(std::string_view filepath, float voxel_size, uint32_t &out_chunk_width, uint32_t &out_chunk_height, uint32_t &out_chunk_depth);
