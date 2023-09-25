#include <cstring>

#include <external/libmorton/include/libmorton/morton.h>

#include "Conversion.h"
#include "utils/Assert.h"

std::vector<std::byte> convert_raw_to_svo(const std::vector<std::byte> &raw, uint32_t width, uint32_t height, uint32_t depth, uint32_t bytes_per_voxel) {
    struct SVONode {
	uint32_t child_pointer_;
	uint32_t valid_mask_: 8;
	uint32_t leaf_mask_: 8;
    };
    static_assert(sizeof(SVONode) == 8);
    ASSERT(bytes_per_voxel <= sizeof(SVONode), "Can't convert a raw chunk to an SVO chunk whose voxels take up more space than a parent SVO node.");
    const SVONode EMPTY_SVO_NODE = [](){ SVONode node; memset(&node, 0, sizeof(SVONode)); return node; }();
    auto nodes_equal = [&](const SVONode &a, const SVONode &b){ return memcmp(&a, &b, sizeof(SVONode)) == 0; };
    auto is_node_empty = [&](const SVONode &node){ return nodes_equal(node, EMPTY_SVO_NODE); };
    
    const double power_of_two = log2(static_cast<double>(width));
    ASSERT(width == height && height == depth && power_of_two == round(power_of_two), "Can't convert a raw chunk to an SVO chunk if it's not a power of two cube.");

    const uint32_t queue_size = 8;
    const uint32_t num_queues = static_cast<uint32_t>(power_of_two);
    std::vector<std::vector<SVONode>> queues(num_queues);
    for (auto &queue : queues) {
	queue.reserve(queue_size);
    }

    std::vector<std::byte> svo;
    auto push_node_to_svo = [&](SVONode node) {
	for (uint32_t i = 0; i < sizeof(SVONode); ++i) {
	    svo.emplace_back();
	}
	memcpy(&svo.back() - sizeof(SVONode) + 1, &node, sizeof(SVONode));
    };
    
    const uint64_t num_voxels = width * height * depth;
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
	uint_fast32_t x = 0, y = 0, z = 0;
	libmorton::morton3D_64_decode(morton, x, y, z);
	size_t voxel_offset = (x + y * width + z * width * height) * bytes_per_voxel;

	SVONode node {};
	memcpy(&node, &raw[voxel_offset], bytes_per_voxel);

	queues.at(num_queues - 1).push_back(node);
	uint32_t d = num_queues - 1;
	while (d > 0 && queues.at(d).size() == queue_size) {
	    SVONode node {};
	    node.child_pointer_ = svo.size() / sizeof(SVONode);
	    node.valid_mask_ = 0;
	    node.leaf_mask_ = 0;

	    for (uint32_t i = 0; i < queue_size;  ++i) {
		const SVONode &child = queues.at(d).at(i);
		if (!is_node_empty(child)) {
		    node.valid_mask_ |= 1 << (7 - i);
		    push_node_to_svo(child);
		}
	    }

	    queues.at(d).clear();
	    queues.at(d - 1).push_back(node);
	    --d;
	}
    }

    return svo;
}
