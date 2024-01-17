#pragma once

#include "Voxel.h"

std::vector<bool>
compute_void_grid_sub_regions_svdag(const std::vector<std::byte> &raw,
                                    uint32_t bytes_per_voxel,
                                    uint32_t sub_region_size);
