#pragma once

#include "Voxel.h"

struct SVONode {
    uint32_t child_offset_;
    uint32_t valid_mask_ : 8;
    uint32_t leaf_mask_ : 8;
};

struct SVDAGNode {
    uint32_t child_offsets_[8];

    bool operator==(const SVDAGNode &other) const {
	return child_offsets_[0] == other.child_offsets_[0] &&
	    child_offsets_[1] == other.child_offsets_[1] &&
	    child_offsets_[2] == other.child_offsets_[2] &&
	    child_offsets_[3] == other.child_offsets_[3] &&
	    child_offsets_[4] == other.child_offsets_[4] &&
	    child_offsets_[5] == other.child_offsets_[5] &&
	    child_offsets_[6] == other.child_offsets_[6] &&
	    child_offsets_[7] == other.child_offsets_[7];
    }
};

static constexpr uint32_t SVDAG_INVALID_OFFSET = 0xFFFFFFFF;

struct HashSVDAGNode {
    size_t operator()(const SVDAGNode &node) const {
	return node.child_offsets_[0] ^
	    (node.child_offsets_[1] << 4) ^
	    (node.child_offsets_[2] << 8) ^
	    (node.child_offsets_[3] << 12) ^
	    (node.child_offsets_[4] << 16) ^
	    (node.child_offsets_[5] << 20) ^
	    (node.child_offsets_[6] << 24) ^
	    (node.child_offsets_[7] << 28);
    }
};

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
