#pragma once

#include "Voxel.h"

std::vector<std::byte> append_metadata_to_raw(const std::vector<std::byte> &raw,
                                              uint32_t width, uint32_t height,
                                              uint32_t depth);

std::vector<std::byte> convert_raw_to_svo(const std::vector<std::byte> &raw,
                                          uint32_t width, uint32_t height,
                                          uint32_t depth,
                                          uint32_t bytes_per_voxel);

void debug_print_svo(const std::vector<std::byte> &svo,
                     uint32_t bytes_per_voxel);

std::vector<std::byte> convert_raw_to_svdag(const std::vector<std::byte> &raw,
					    uint32_t width, uint32_t height,
					    uint32_t depth, uint32_t bytes_per_voxel);

void debug_print_svdag(const std::vector<std::byte> &svdag,
		       uint32_t bytes_per_voxel);

std::vector<std::byte> convert_raw_color_to_df(const std::vector<std::byte> &raw,
					       uint32_t width, uint32_t height,
					       uint32_t depth, uint32_t max_dist);
