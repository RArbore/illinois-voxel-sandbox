#include <cstdint>
#include <fstream>
#include <vector>
#include <array>
#include <map>

#include <external/libmorton/include/libmorton/morton.h>

#include "Voxelize.h"

template<class T> 
struct std::hash<std::vector<T>> {
    auto operator() (const std::vector<T>& key) const {
        std::hash<T> hasher;
        size_t result = 0;
        for(size_t i = 0; i < key.size(); ++i) {
            result = result * 31 + hasher(key[i]);
        }
        return result;
    }
};

static uint32_t push_node_to_buffer(std::pair<std::ofstream &, uint32_t &> buffer, uint32_t node) {
    return node;
}

static uint32_t push_node_to_buffer(std::pair<std::ofstream &, uint32_t &> buffer, const std::vector<uint32_t> &node) {
    uint32_t offset = buffer.second;
    buffer.first.write(reinterpret_cast<const char *>(node.data()), node.size() * sizeof(uint32_t));
    buffer.second += node.size();
    return offset;
}

static uint32_t push_node_to_buffer(std::pair<std::ofstream &, uint32_t &> buffer, const std::array<uint32_t, 2> &node) {
    uint32_t offset = buffer.second;
    buffer.first.write(reinterpret_cast<const char *>(node.data()), node.size() * sizeof(uint32_t));
    buffer.second += node.size();
    return offset;
}

static std::vector<uint32_t> raw_16_16_16_raw_2_2_2_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> raw_2_2_2_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

void raw_16_16_16_raw_2_2_2_raw_16_16_16_construct(Voxelizer &voxelizer, std::ofstream &buffer) {
    bool is_empty;
    uint32_t size = 0;
    buffer.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    ++size;
    auto root_node = raw_16_16_16_raw_2_2_2_raw_16_16_16_construct_node(voxelizer, {buffer, size}, 0, 0, 0, is_empty);
    uint32_t root = push_node_to_buffer({buffer, size}, root_node);
    buffer.seekp(0, std::ios_base::beg);
    buffer.write(reinterpret_cast<const char *>(&root), sizeof(uint32_t));
    buffer.seekp(0, std::ios_base::end);
}

static std::vector<uint32_t> raw_16_16_16_raw_2_2_2_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    is_empty = true;
    uint64_t num_voxels = 4096;
    std::vector<uint32_t> raw_chunk(num_voxels);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 16 + g_z * 16 * 16;
        uint32_t sub_lower_x = lower_x + g_x * 32;
        uint32_t sub_lower_y = lower_y + g_y * 32;
        uint32_t sub_lower_z = lower_z + g_z * 32;
        bool sub_is_empty;
        auto sub_chunk = raw_2_2_2_raw_16_16_16_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            raw_chunk.at(linear_idx) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    return raw_chunk;
}

static std::vector<uint32_t> raw_2_2_2_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    is_empty = true;
    uint64_t num_voxels = 8;
    std::vector<uint32_t> raw_chunk(num_voxels);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 2 + g_z * 2 * 2;
        uint32_t sub_lower_x = lower_x + g_x * 16;
        uint32_t sub_lower_y = lower_y + g_y * 16;
        uint32_t sub_lower_z = lower_z + g_z * 16;
        bool sub_is_empty;
        auto sub_chunk = raw_16_16_16_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            raw_chunk.at(linear_idx) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    return raw_chunk;
}

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    is_empty = true;
    uint64_t num_voxels = 4096;
    std::vector<uint32_t> raw_chunk(num_voxels);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 16 + g_z * 16 * 16;
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

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}
