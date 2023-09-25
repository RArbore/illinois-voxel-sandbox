#pragma once

#include "Voxel.h"

std::vector<std::byte> convert_raw_to_svo(const std::vector<std::byte> &raw, uint32_t width, uint32_t height, uint32_t depth, uint32_t bytes_per_voxel);
