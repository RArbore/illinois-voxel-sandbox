#include <cstdint>
#include <fstream>
#include <vector>
#include <array>
#include <map>

#include <external/libmorton/include/libmorton/morton.h>

#include "Voxelize.h"

template<class T, size_t N> 
struct std::hash<std::array<T, N>> {
    auto operator() (const std::array<T, N>& key) const {
        std::hash<T> hasher;
        size_t result = 0;
        for(size_t i = 0; i < N; ++i) {
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

static uint32_t push_node_to_buffer(std::pair<std::ofstream &, uint32_t &> buffer, const std::array<uint32_t, 8> &node) {
    uint32_t offset = buffer.second;
    buffer.first.write(reinterpret_cast<const char *>(node.data()), node.size() * sizeof(uint32_t));
    buffer.second += node.size();
    return offset;
}

static std::vector<uint32_t> df_8_8_8_6_df_8_8_8_6_svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> df_8_8_8_6_svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::array<uint32_t, 8> svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty, std::unordered_map<std::array<uint32_t, 8>, uint32_t> &deduplication_map);

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

void df_8_8_8_6_df_8_8_8_6_svdag_3_construct(Voxelizer &voxelizer, std::ofstream &buffer) {
    bool is_empty;
    uint32_t size = 0;
    buffer.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    ++size;
    auto root_node = df_8_8_8_6_df_8_8_8_6_svdag_3_construct_node(voxelizer, {buffer, size}, 0, 0, 0, is_empty);
    uint32_t root = push_node_to_buffer({buffer, size}, root_node);
    buffer.seekp(0, std::ios_base::beg);
    buffer.write(reinterpret_cast<const char *>(&root), sizeof(uint32_t));
    buffer.seekp(0, std::ios_base::end);
}

static std::vector<uint32_t> df_8_8_8_6_df_8_8_8_6_svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    is_empty = true;
    uint64_t num_voxels = 512;
    std::vector<uint32_t> df_chunk(num_voxels * 1);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 8 + g_z * 8 * 8;
        uint32_t sub_lower_x = lower_x + g_x * 64;
        uint32_t sub_lower_y = lower_y + g_y * 64;
        uint32_t sub_lower_z = lower_z + g_z * 64;
        bool sub_is_empty;
        auto sub_chunk = df_8_8_8_6_svdag_3_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            df_chunk.at(linear_idx * 1) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    uint32_t k = 6;
    for (uint32_t g_z = 0; g_z < 8; ++g_z) {
        for (uint32_t g_y = 0; g_y < 8; ++g_y) {
            for (uint32_t g_x = 0; g_x < 8; ++g_x) {
                uint32_t g_voxel_offset = (g_x + g_y * 8 + g_z * 8 * 8);
                uint32_t min_dist = k;
                for (uint32_t kz = g_z > k ? g_z - k : 0; kz <= g_z + k && kz < 8; ++kz) {
                    for (uint32_t ky = g_y > k ? g_y - k : 0; ky <= g_y + k && ky < 8; ++ky) {
                        for (uint32_t kx = g_x > k ? g_x - k : 0; kx <= g_x + k && kx < 8; ++kx) {
                            size_t k_voxel_offset = kx + ky * 8 + kz * 8 * 8;
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

static std::vector<uint32_t> df_8_8_8_6_svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::unordered_map<std::array<uint32_t, 8>, uint32_t> deduplication_map;

    is_empty = true;
    uint64_t num_voxels = 512;
    std::vector<uint32_t> df_chunk(num_voxels * 1);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 8 + g_z * 8 * 8;
        uint32_t sub_lower_x = lower_x + g_x * 8;
        uint32_t sub_lower_y = lower_y + g_y * 8;
        uint32_t sub_lower_z = lower_z + g_z * 8;
        bool sub_is_empty;
        auto sub_chunk = svdag_3_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty, deduplication_map);
        if (!sub_is_empty) {
            df_chunk.at(linear_idx * 1) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    uint32_t k = 6;
    for (uint32_t g_z = 0; g_z < 8; ++g_z) {
        for (uint32_t g_y = 0; g_y < 8; ++g_y) {
            for (uint32_t g_x = 0; g_x < 8; ++g_x) {
                uint32_t g_voxel_offset = (g_x + g_y * 8 + g_z * 8 * 8);
                uint32_t min_dist = k;
                for (uint32_t kz = g_z > k ? g_z - k : 0; kz <= g_z + k && kz < 8; ++kz) {
                    for (uint32_t ky = g_y > k ? g_y - k : 0; ky <= g_y + k && ky < 8; ++ky) {
                        for (uint32_t kx = g_x > k ? g_x - k : 0; kx <= g_x + k && kx < 8; ++kx) {
                            size_t k_voxel_offset = kx + ky * 8 + kz * 8 * 8;
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

static std::array<uint32_t, 8> svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty, std::unordered_map<std::array<uint32_t, 8>, uint32_t> &deduplication_map) {
    uint32_t power_of_two = 3;
    const uint64_t bounded_edge_length = 1 << power_of_two;
    std::vector<std::vector<std::array<uint32_t, 8>>> queues(power_of_two + 1);
    const uint64_t num_voxels = bounded_edge_length * bounded_edge_length * bounded_edge_length;

    auto nodes_equal = [](const std::array<uint32_t, 8> &a, const std::array<uint32_t, 8> &b) {
        return a == b;
    };

    auto is_node_empty = [](const std::array<uint32_t, 8> &node) {
        return !node[0] && !node[1] && !node[2] && !node[3] && !node[4] && !node[5] && !node[6] && !node[7];
    };

    is_empty = true;

    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

        std::array<uint32_t, 8> node = {0, 0, 0, 0, 0, 0, 0, 0};
        uint32_t sub_lower_x = x * 1 + lower_x, sub_lower_y = y * 1 + lower_y, sub_lower_z = z * 1 + lower_z;
        bool sub_is_empty;
        auto sub_chunk = _construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            node[0] = push_node_to_buffer(buffer, sub_chunk);
            node[1] = 0xFFFFFFFF;
            is_empty = false;
        }

        queues.at(power_of_two).emplace_back(node);
        uint32_t d = power_of_two;
        while (d > 0 && queues.at(d).size() == 8) {
            std::array<uint32_t, 8> node = {0, 0, 0, 0, 0, 0, 0, 0};

            bool identical = true;
            for (uint32_t i = 0; i < 8; ++i) {
                identical = identical && nodes_equal(queues.at(d).at(i), queues.at(d).at(0));
            }

            if (identical) {
                node = queues.at(d).at(0);
            } else {
                for (uint32_t i = 0; i < 8; ++i) {
                    auto child = queues.at(d).at(i);
                    if (!is_node_empty(child)) {
                        if (deduplication_map.contains(child)) {
                            node[i] = deduplication_map[child];
                        } else {
                            uint32_t child_idx = push_node_to_buffer(buffer, child);
                            node[i] = child_idx;
                            deduplication_map.emplace(child, child_idx);
                        }
                    }
                }
            }

            queues.at(d - 1).emplace_back(node);
            queues.at(d).clear();
            --d;
        }
    }

    return queues.at(0).at(0);
}

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}
