#include <cstring>
#include <math.h>

#include "VoidGrid.h"
#include "Conversion.h"
#include "utils/Assert.h"

static bool is_zero(const std::byte *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
	if (static_cast<bool>(data[i])) {
	    return false;
	}
    }
    return true;
}

static void compute_void_grid_sub_regions_svdag_helper(const std::vector<std::byte> &svdag,
						       uint32_t bytes_per_voxel,
						       uint32_t sub_region_size,
						       uint32_t node_idx,
						       uint32_t log_sub_region_size,
						       uint32_t log_current_size,
						       std::vector<bool> &result) {
    if (log_current_size == log_sub_region_size) {
	result.push_back(false);
	return;
    }

    SVDAGNode node;
    memcpy(&node, svdag.data() + 4 * sizeof(uint32_t) + node_idx * sizeof(SVDAGNode), sizeof(SVDAGNode));
    for (uint32_t child = 0; child < 8; ++child) {
	uint32_t adjusted_child_idx = node.child_offsets_[child] & 0x7FFFFFFF;
	bool is_leaf = node.child_offsets_[child] >> 31;
	bool is_valid = node.child_offsets_[child] != SVDAG_INVALID_OFFSET;
	if (!is_valid) {
	    for (uint32_t region = 0; region < 1 << (log_current_size - log_sub_region_size - 1); ++region) {
		result.push_back(true);
	    }
	} else if (is_leaf) {
	    bool is_empty = is_zero(svdag.data() + 4 * sizeof(uint32_t) + adjusted_child_idx * sizeof(SVDAGNode), bytes_per_voxel);
	    for (uint32_t region = 0; region < 1 << (log_current_size - log_sub_region_size - 1); ++region) {
		result.push_back(is_empty);
	    }
	} else {
	    compute_void_grid_sub_regions_svdag_helper(svdag, bytes_per_voxel, sub_region_size, adjusted_child_idx, log_sub_region_size, log_current_size - 1, result);
	}
    }
}

std::vector<bool> compute_void_grid_sub_regions_svdag(const std::vector<std::byte> &svdag,
						      uint32_t bytes_per_voxel,
						      uint32_t sub_region_size) {
    ASSERT(1 << static_cast<uint32_t>(log2(static_cast<double>(sub_region_size))) == sub_region_size, "Size of potentially void chunks must be a power of two.");

    uint32_t header[4];
    memcpy(header, svdag.data(), sizeof(header));
    const uint32_t width = header[0];
    const uint32_t height = header[1];
    const uint32_t depth = header[2];

    const uint32_t log_max_dim = ceil(
        log2(static_cast<double>(width > height ? width > depth ? width : depth
                                 : height > depth ? height
                                                  : depth)));

    const uint32_t log_sub_region_size = ceil(log2(static_cast<double>(sub_region_size)));

    if (header[3] == 1) {
	return {is_zero(svdag.data() + 4 * sizeof(uint32_t), bytes_per_voxel)};
    }

    std::vector<bool> result;
    compute_void_grid_sub_regions_svdag_helper(svdag, bytes_per_voxel, sub_region_size, (svdag.size() - sizeof(uint32_t) * 4) / sizeof(SVDAGNode) - 1, log_sub_region_size, log_max_dim, result);
    return result;
}
