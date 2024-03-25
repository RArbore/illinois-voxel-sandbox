#include <cstdint>
#include <vector>
#include <array>
#include <map>

#include <external/libmorton/include/libmorton/morton.h>

#include "Voxelize.h"

static uint32_t push_node_to_buffer(std::vector<uint32_t> &buffer, uint32_t node) {
    return node;
}

static uint32_t push_node_to_buffer(std::vector<uint32_t> &buffer, const std::vector<uint32_t> &node) {
    uint32_t offset = buffer.size();
    buffer.insert(buffer.end(), node.cbegin(), node.cend());
    return offset;
}

static uint32_t push_node_to_buffer(std::vector<uint32_t> &buffer, const std::array<uint32_t, 2> &node) {
    uint32_t offset = buffer.size();
    buffer.insert(buffer.end(), node.cbegin(), node.cend());
    return offset;
}

static uint32_t push_node_to_buffer(std::vector<uint32_t> &buffer, const std::array<uint32_t, 8> &node) {
    uint32_t offset = buffer.size();
    buffer.insert(buffer.end(), node.cbegin(), node.cend());
    return offset;
}

static std::vector<uint32_t> raw_512_512_512_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

std::vector<uint32_t> raw_512_512_512_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer {0};
    bool is_empty;
    auto root_node = raw_512_512_512_construct_node(voxelizer, buffer, 0, 0, 0, is_empty);
    buffer.at(0) = push_node_to_buffer(buffer, root_node);
    return buffer;
}

static std::vector<uint32_t> raw_512_512_512_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    is_empty = true;
    uint64_t num_voxels = 134217728;
    std::vector<uint32_t> raw_chunk(num_voxels);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 512 + g_z * 512 * 512;
        uint32_t sub_lower_x = lower_x + g_x * 1;
        uint32_t sub_lower_y = lower_y + g_y * 1;
        uint32_t sub_lower_z = lower_z + g_z * 1;
        bool sub_is_empty;
        auto sub_chunk = _construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            raw_chunk.at(linear_idx) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    return raw_chunk;
}

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}
