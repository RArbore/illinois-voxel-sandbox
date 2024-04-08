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

static std::vector<uint32_t> df_32_32_32_6_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

void df_32_32_32_6_raw_16_16_16_construct(Voxelizer &voxelizer, std::ofstream &buffer) {
    bool is_empty;
    uint32_t size = 0;
    buffer.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    ++size;
    auto root_node = df_32_32_32_6_raw_16_16_16_construct_node(voxelizer, {buffer, size}, 0, 0, 0, is_empty);
    uint32_t root = push_node_to_buffer({buffer, size}, root_node);
    buffer.seekp(0, std::ios_base::beg);
    buffer.write(reinterpret_cast<const char *>(&root), sizeof(uint32_t));
    buffer.seekp(0, std::ios_base::end);
}

static std::vector<uint32_t> df_32_32_32_6_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    is_empty = true;
    uint64_t num_voxels = 32768;
    std::vector<uint32_t> df_chunk(num_voxels * 1);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 32 + g_z * 32 * 32;
        uint32_t sub_lower_x = lower_x + g_x * 16;
        uint32_t sub_lower_y = lower_y + g_y * 16;
        uint32_t sub_lower_z = lower_z + g_z * 16;
        bool sub_is_empty;
        auto sub_chunk = raw_16_16_16_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            df_chunk.at(linear_idx * 1) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    uint32_t k = 6;
    for (uint32_t g_z = 0; g_z < 32; ++g_z) {
        for (uint32_t g_y = 0; g_y < 32; ++g_y) {
            for (uint32_t g_x = 0; g_x < 32; ++g_x) {
                uint32_t g_voxel_offset = (g_x + g_y * 32 + g_z * 32 * 32);
                uint32_t min_dist = k;
                for (uint32_t kz = g_z > k ? g_z - k : 0; kz <= g_z + k && kz < 32; ++kz) {
                    for (uint32_t ky = g_y > k ? g_y - k : 0; ky <= g_y + k && ky < 32; ++ky) {
                        for (uint32_t kx = g_x > k ? g_x - k : 0; kx <= g_x + k && kx < 32; ++kx) {
                            size_t k_voxel_offset = kx + ky * 32 + kz * 32 * 32;
                            if (df_chunk[k_voxel_offset] & 268435455) {
                                uint32_t dist =
                                (g_z > kz ? g_z - kz : kz - g_z) +
                                (g_y > ky ? g_y - ky : ky - g_y) +
                                (g_x > kx ? g_x - kx : kx - g_x);
                                if (dist < min_dist && dist) {
                                    min_dist = dist;
                                }
                            }
                        }
                    }
                }
                df_chunk[g_voxel_offset] |= (min_dist - 1) << (32 - 4);
            }
        }
    }
    return df_chunk;
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
