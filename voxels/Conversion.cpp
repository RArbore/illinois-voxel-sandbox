#include <cstring>

#include <external/libmorton/include/libmorton/morton.h>

#include "Conversion.h"
#include "utils/Assert.h"

std::vector<std::byte> convert_raw_to_svo(const std::vector<std::byte> &raw,
                                          uint32_t width, uint32_t height,
                                          uint32_t depth,
                                          uint32_t bytes_per_voxel) {
    static_assert(sizeof(SVONode) == 8);
    ASSERT(bytes_per_voxel <= sizeof(SVONode),
           "Can't convert a raw chunk to an SVO chunk whose voxels take up "
           "more space than an internal SVO node.");

    const SVONode EMPTY_SVO_NODE = []() {
        SVONode node;
        memset(&node, 0, sizeof(SVONode));
        return node;
    }();
    auto nodes_equal = [&](const SVONode &a, const SVONode &b) {
        return memcmp(&a, &b, sizeof(SVONode)) == 0;
    };
    auto is_node_empty = [&](const SVONode &node) {
        return nodes_equal(node, EMPTY_SVO_NODE);
    };

    const uint32_t power_of_two = ceil(
        log2(static_cast<double>(width > height ? width > depth ? width : depth
                                 : height > depth ? height
                                                  : depth)));
    const uint32_t bounded_edge_length = 1 << power_of_two;

    const uint32_t queue_size = 8;
    const uint32_t num_queues = power_of_two + 1;
    std::vector<std::vector<std::pair<SVONode, bool>>> queues(num_queues);
    for (auto &queue : queues) {
        queue.reserve(queue_size);
    }

    std::vector<std::byte> svo;
    for (uint32_t i = 0; i < sizeof(uint32_t) * 4; ++i) {
        svo.emplace_back();
    }
    memcpy(&svo.at(0), &width, sizeof(uint32_t));
    memcpy(&svo.at(sizeof(uint32_t)), &height, sizeof(uint32_t));
    memcpy(&svo.at(sizeof(uint32_t) * 2), &depth, sizeof(uint32_t));

    auto push_node_to_svo = [&](SVONode node, bool internal) {
	if (internal) {
	    uint32_t new_size = static_cast<uint32_t>((svo.size() - sizeof(uint32_t) * 4) / sizeof(SVONode));
	    node.child_offset_ = new_size - node.child_offset_;
	}
        for (uint32_t i = 0; i < sizeof(SVONode); ++i) {
            svo.emplace_back();
        }
        memcpy(&svo.back() - sizeof(SVONode) + 1, &node, sizeof(SVONode));
    };

    const uint64_t num_voxels =
        bounded_edge_length * bounded_edge_length * bounded_edge_length;
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

        SVONode node = EMPTY_SVO_NODE;
        if (x < width && y < height && z < depth) {
            size_t voxel_offset =
                (x + y * width + z * width * height) * bytes_per_voxel;
            memcpy(&node, &raw[voxel_offset], bytes_per_voxel);
        }

        queues.at(num_queues - 1).emplace_back(node, true);
        uint32_t d = num_queues - 1;
        while (d > 0 && queues.at(d).size() == queue_size) {
            SVONode node{};
            node.child_offset_ = static_cast<uint32_t>(
                (svo.size() - sizeof(uint32_t) * 4) / sizeof(SVONode));
            node.valid_mask_ = 0;
            node.leaf_mask_ = 0;

            bool identical = true;
            for (uint32_t i = 0; i < queue_size; ++i) {
                node.valid_mask_ |= !is_node_empty(queues.at(d).at(i).first)
                                    << (7 - i);
                node.leaf_mask_ |= queues.at(d).at(i).second << (7 - i);
                identical =
                    identical &&
                    nodes_equal(queues.at(d).at(i).first,
                                queues.at(d).at(0).first) &&
                    queues.at(d).at(i).second == queues.at(d).at(0).second;
            }
            node.leaf_mask_ &= node.valid_mask_;

            if (identical) {
                node = queues.at(d).at(0).first;
            } else {
                for (uint32_t i = 0; i < queue_size; ++i) {
                    const SVONode &child = queues.at(d).at(i).first;
                    if (!is_node_empty(child)) {
                        push_node_to_svo(child, !static_cast<bool>(node.leaf_mask_ & (1 << (7 - i))));
                    }
                }
            }

            queues.at(d - 1).emplace_back(
                node, identical ? queues.at(d).at(0).second : false);
            queues.at(d).clear();
            --d;
        }
    }
    push_node_to_svo(queues.at(0).at(0).first, true);

    const uint32_t num_nodes = static_cast<uint32_t>(
        (svo.size() - sizeof(uint32_t) * 4) / sizeof(SVONode));
    memcpy(&svo.at(sizeof(uint32_t) * 3), &num_nodes, sizeof(uint32_t));

    return svo;
}

static void debug_print_svo_leaf_helper(const std::span<const std::byte> &svo,
                                    uint32_t bytes_per_voxel, uint32_t level) {
    for (uint32_t i = 0; i < level; ++i) {
        std::cout << " ";
    }

    std::cout << "Leaf Node:";
    for (size_t i = svo.size() - sizeof(SVONode);
         i < svo.size() - sizeof(SVONode) + bytes_per_voxel; ++i) {
        uint32_t conv = std::to_integer<uint32_t>(svo[i]);
        std::cout << " " << conv;
    }
    std::cout << "\n";
}

static void debug_print_svo_internal_helper(const std::span<const std::byte> &svo,
                                        uint32_t bytes_per_voxel,
                                        uint32_t level) {
    for (uint32_t i = 0; i < level; ++i) {
        std::cout << " ";
    }

    SVONode node;
    memcpy(&node, svo.data() + svo.size() - sizeof(SVONode), sizeof(SVONode));
    std::cout << "Internal Node: ";
    for (uint32_t i = 0; i < 8; ++i) {
        if (node.valid_mask_ & (1 << (7 - i))) {
            std::cout << "1";
        } else {
            std::cout << "0";
        }
    }
    std::cout << " ";
    for (uint32_t i = 0; i < 8; ++i) {
        if (node.leaf_mask_ & (1 << (7 - i))) {
            std::cout << "1";
        } else {
            std::cout << "0";
        }
    }
    std::cout << " " << node.child_offset_ << "\n";

    for (uint32_t i = 0, j = 0; i < 8; ++i) {
	uint32_t length = svo.size() + j * sizeof(SVONode) - node.child_offset_ * sizeof(SVONode);
        if (node.valid_mask_ & (1 << (7 - i)) &&
            node.leaf_mask_ & (1 << (7 - i))) {
            debug_print_svo_leaf_helper(svo.subspan(0, length),
                                    bytes_per_voxel, level + 1);
	    ++j;
        } else if (node.valid_mask_ & (1 << (7 - i))) {
            debug_print_svo_internal_helper(
                svo.subspan(0, length),
                bytes_per_voxel, level + 1);
	    ++j;
        }
    }
}

void debug_print_svo(const std::vector<std::byte> &svo,
                     uint32_t bytes_per_voxel) {
    ASSERT(svo.size() >= sizeof(uint32_t) * 4 + sizeof(SVONode),
           "Can't debug print a malformed SVO.");
    std::cout << "INFO: Debug printing a " << svo.size() << " byte SVO.\n";
    uint32_t header[4];
    memcpy(header, svo.data(), sizeof(header));
    std::cout << "SVO represent a " << header[0] << " x " << header[1] << " x "
              << header[2] << " volume. Its self reported size is " << header[3]
              << " node(s).\n";
    if (header[3] > 1) {
        debug_print_svo_internal_helper(std::span(svo), bytes_per_voxel, 0);
    } else {
        debug_print_svo_leaf_helper(std::span(svo), bytes_per_voxel, 0);
    }
}

std::vector<std::byte> convert_raw_to_svdag(const std::vector<std::byte> &raw,
					    uint32_t width, uint32_t height,
					    uint32_t depth, uint32_t bytes_per_voxel) {
    static_assert(sizeof(SVDAGNode) == 32);
    ASSERT(bytes_per_voxel <= sizeof(SVDAGNode),
           "Can't convert a raw chunk to an SVDAG chunk whose voxels take up "
           "more space than an internal SVDAG node.");

    const SVDAGNode EMPTY_SVDAG_NODE = []() {
        SVDAGNode node;
        memset(&node, 0, sizeof(SVDAGNode));
        return node;
    }();
    auto nodes_equal = [&](const SVDAGNode &a, const SVDAGNode &b) {
        return memcmp(&a, &b, sizeof(SVDAGNode)) == 0;
    };
    auto is_node_empty = [&](const SVDAGNode &node) {
        return nodes_equal(node, EMPTY_SVDAG_NODE);
    };

    const uint32_t power_of_two = ceil(
        log2(static_cast<double>(width > height ? width > depth ? width : depth
                                 : height > depth ? height
                                                  : depth)));
    const uint64_t bounded_edge_length = 1 << power_of_two;

    const uint32_t queue_size = 8;
    const uint32_t num_queues = power_of_two + 1;
    std::vector<std::vector<std::pair<SVDAGNode, bool>>> queues(num_queues);
    for (auto &queue : queues) {
        queue.reserve(queue_size);
    }

    std::vector<std::byte> svdag;
    std::unordered_map<SVDAGNode, uint32_t, HashSVDAGNode> already_inserted_nodes;
    for (uint32_t i = 0; i < sizeof(uint32_t) * 4; ++i) {
        svdag.emplace_back();
    }
    memcpy(&svdag.at(0), &width, sizeof(uint32_t));
    memcpy(&svdag.at(sizeof(uint32_t)), &height, sizeof(uint32_t));
    memcpy(&svdag.at(sizeof(uint32_t) * 2), &depth, sizeof(uint32_t));

    auto push_node_to_svdag = [&](SVDAGNode node) {
	ASSERT(!is_node_empty(node), "Attempted to push an empty SVDAG node.");
	if (already_inserted_nodes.contains(node)) {
	    return already_inserted_nodes.at(node);
	} else {
	    uint32_t offset = static_cast<uint32_t>(svdag.size() - sizeof(uint32_t) * 4) / sizeof(SVDAGNode);
	    for (uint32_t i = 0; i < sizeof(SVDAGNode); ++i) {
		svdag.emplace_back();
	    }
	    memcpy(&svdag.back() - sizeof(SVDAGNode) + 1, &node, sizeof(SVDAGNode));
	    already_inserted_nodes.emplace(node, offset);
	    return offset;
	}
    };

    const uint64_t num_voxels =
        bounded_edge_length * bounded_edge_length * bounded_edge_length;
    uint32_t num_printed = 0;
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

	SVDAGNode leaf_node = EMPTY_SVDAG_NODE;
	if (x < width && y < height && z < depth) {
            size_t voxel_offset =
                (x + y * width + z * width * height) * bytes_per_voxel;
            memcpy(&leaf_node, &raw[voxel_offset], bytes_per_voxel);
	}

        queues.at(num_queues - 1).emplace_back(leaf_node, true);
        uint32_t d = num_queues - 1;
        while (d > 0 && queues.at(d).size() == queue_size) {
	    bool identical = true;
	    for (uint32_t i = 0; i < queue_size; ++i) {
		identical = identical && nodes_equal(queues.at(d).at(i).first,
						     queues.at(d).at(0).first);
	    }

	    SVDAGNode node{};
	    if (identical) {
		node = queues.at(d).at(0).first;
	    } else {
		for (uint32_t i = 0; i < queue_size; ++i) {
                    const SVDAGNode &child = queues.at(d).at(i).first;
		    const bool is_leaf = queues.at(d).at(i).second;
                    if (!is_node_empty(child)) {
                        uint32_t offset = push_node_to_svdag(child);
			node.child_offsets_[i] = offset | (static_cast<uint32_t>(is_leaf) << 31);
		    } else {
			node.child_offsets_[i] = SVDAG_INVALID_OFFSET;
		    }
		}
	    }

            queues.at(d - 1).emplace_back(
                node, identical ? queues.at(d).at(0).second : false);
            queues.at(d).clear();
            --d;
        }
	if (morton * 100 / num_voxels - num_printed >= 10) {
	    num_printed = morton * 100 / num_voxels;
	    std::cout << num_printed << "% finished.\n";
	}
    }
    std::cout << "100% finished.\n";
    uint32_t root = push_node_to_svdag(queues.at(0).at(0).first);

    const uint32_t num_nodes = static_cast<uint32_t>(
        (svdag.size() - sizeof(uint32_t) * 4) / sizeof(SVDAGNode));
    ASSERT(root + 1 == num_nodes, "Root node in SVDAG is not the last node.");

    memcpy(&svdag.at(sizeof(uint32_t) * 3), &num_nodes, sizeof(uint32_t));

    return svdag;
}

static void debug_print_svdag_leaf_helper(const std::span<const std::byte> &svdag,
					  uint32_t bytes_per_voxel, uint32_t level) {
    for (uint32_t i = 0; i < level; ++i) {
        std::cout << " ";
    }
    
    uint32_t current_offset = (svdag.size() - sizeof(uint32_t) * 4) / sizeof(SVDAGNode) - 1;
    std::cout << "Leaf Node @ " << current_offset << ": ";
    for (size_t i = svdag.size() - sizeof(SVDAGNode);
         i < svdag.size() - sizeof(SVDAGNode) + bytes_per_voxel; ++i) {
        uint32_t conv = std::to_integer<uint32_t>(svdag[i]);
        std::cout << " " << conv;
    }
    std::cout << "\n";
}

static void debug_print_svdag_internal_helper(const std::span<const std::byte> &svdag,
					      uint32_t bytes_per_voxel,
					      uint32_t level) {
    for (uint32_t i = 0; i < level; ++i) {
        std::cout << " ";
    }
    
    SVDAGNode node;
    memcpy(&node, svdag.data() + svdag.size() - sizeof(SVDAGNode), sizeof(SVDAGNode));
    uint32_t current_offset = (svdag.size() - sizeof(uint32_t) * 4) / sizeof(SVDAGNode) - 1;
    std::cout << "Internal Node @ " << current_offset << ": ";
    for (uint32_t i = 0; i < 8; ++i) {
        if (node.child_offsets_[i] != SVDAG_INVALID_OFFSET) {
            std::cout << "1";
        } else {
            std::cout << "0";
        }
    }
    std::cout << " ";
    for (uint32_t i = 0; i < 8; ++i) {
        if (node.child_offsets_[i] != SVDAG_INVALID_OFFSET && node.child_offsets_[i] >> 31) {
            std::cout << "1";
        } else {
            std::cout << "0";
        }
    }

    for (uint32_t i = 0; i < 8; ++i) {
	uint32_t adjusted_child_offset = node.child_offsets_[i] & 0x7FFFFFFF;
	std::cout << " " << (node.child_offsets_[i] == SVDAG_INVALID_OFFSET ? "." : std::to_string(adjusted_child_offset));
    }
    std::cout << "\n";
    
    for (uint32_t i = 0; i < 8; ++i) {
	uint32_t adjusted_child_offset = node.child_offsets_[i] & 0x7FFFFFFF;
	bool is_leaf = node.child_offsets_[i] >> 31;
	bool is_valid = node.child_offsets_[i] != SVDAG_INVALID_OFFSET;
	uint32_t length = sizeof(uint32_t) * 4 + (adjusted_child_offset + 1) * sizeof(SVDAGNode);
	ASSERT(!is_valid || length < svdag.size(), "Debug print failed. Malformed DAG?");
        if (is_valid && is_leaf) {
            debug_print_svdag_leaf_helper(svdag.subspan(0, length),
					  bytes_per_voxel, level + 1);
        } else if (is_valid) {
            debug_print_svdag_internal_helper(
					      svdag.subspan(0, length),
					      bytes_per_voxel, level + 1);
        }
    }
}

void debug_print_svdag(const std::vector<std::byte> &svdag,
		       uint32_t bytes_per_voxel) {
    ASSERT(svdag.size() >= sizeof(uint32_t) * 4 + sizeof(SVDAGNode),
           "Can't debug print a malformed SVDAG.");
    std::cout << "INFO: Debug printing a " << svdag.size() << " byte SVDAG.\n";
    uint32_t header[4];
    memcpy(header, svdag.data(), sizeof(header));
    std::cout << "SVDAG represent a " << header[0] << " x " << header[1] << " x "
              << header[2] << " volume. Its self reported size is " << header[3]
              << " node(s).\n";
    if (header[3] > 1) {
        debug_print_svdag_internal_helper(std::span(svdag), bytes_per_voxel, 0);
    } else {
        debug_print_svdag_leaf_helper(std::span(svdag), bytes_per_voxel, 0);
    }
}
