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

static std::array<uint32_t, 2> svo_8_df_16_16_16_12_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> df_16_16_16_12_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

std::vector<uint32_t> svo_8_df_16_16_16_12_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer {0};
    bool is_empty;
    auto root_node = svo_8_df_16_16_16_12_construct_node(voxelizer, buffer, 0, 0, 0, is_empty);
    buffer.at(0) = push_node_to_buffer(buffer, root_node);
    return buffer;
}

static std::array<uint32_t, 2> svo_8_df_16_16_16_12_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t power_of_two = 8;
    const uint64_t bounded_edge_length = 1 << power_of_two;
    std::vector<std::vector<std::array<uint32_t, 2>>> queues(power_of_two + 1);
    const uint64_t num_voxels = bounded_edge_length * bounded_edge_length * bounded_edge_length;

    auto nodes_equal = [](const std::array<uint32_t, 2> &a, const std::array<uint32_t, 2> &b) {
        return a[0] == b[0] && a[1] == b[1];
    };

    auto is_node_empty = [](const std::array<uint32_t, 2> &node) {
        return !node[0] && !node[1];
    };

    auto is_node_leaf = [](const std::array<uint32_t, 2> &node) {
        return !node[1];
    };

    is_empty = true;
    uint64_t current_morton = 0;

    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

        std::array<uint32_t, 2> node = {0, 0};
        uint32_t sub_lower_x = x + lower_x, sub_lower_y = y + lower_y, sub_lower_z = z + lower_z;
        bool sub_is_empty;
        auto sub_chunk = df_16_16_16_12_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            node[0] = push_node_to_buffer(buffer, sub_chunk);
            is_empty = false;

            int64_t nb_empty_nodes = morton - current_morton;
            while (nb_empty_nodes > 0) {
                uint64_t a = (63 - std::countl_zero((uint64_t) nb_empty_nodes)) / 3;
                int64_t b;
                for (b = power_of_two; b > 0 && queues.at(b).empty(); --b);
                uint32_t d = power_of_two - a > b ? power_of_two - a : b;
                queues.at(d).emplace_back();
                while (d > 0 && queues.at(d).size() == 8) {
                    std::array<uint32_t, 2> node = {0, 0};
                    
                    bool identical = true;
                    for (uint32_t i = 0; i < 8; ++i) {
                        bool child_is_valid = !is_node_empty(queues.at(d).at(i));
                        bool child_is_leaf = is_node_leaf(queues.at(d).at(i));
                        node[1] |= child_is_valid << (7 - i);
                        node[1] |= (child_is_valid && child_is_leaf) << (15 - i);
                        identical = identical && nodes_equal(queues.at(d).at(i), queues.at(d).at(0));
                    }
                    
                    if (identical) {
                        node = queues.at(d).at(0);
                    } else {
                        uint32_t first_child_idx = 0;
                        for (uint32_t i = 0; i < 8; ++i) {
                            auto child = queues.at(d).at(i);
                            if (!is_node_empty(child)) {
                                uint32_t child_idx = push_node_to_buffer(buffer, child);
                                first_child_idx = first_child_idx ? first_child_idx : child_idx;
                            }
                        }
                        node[0] = first_child_idx;
                    }
                    
                    queues.at(d - 1).emplace_back(node);
                    queues.at(d).clear();
                    --d;
                }

                nb_empty_nodes -= 1 << ((power_of_two - d) * 3);
            }

            queues.at(power_of_two).emplace_back(node);
            uint32_t d = power_of_two;
            while (d > 0 && queues.at(d).size() == 8) {
                std::array<uint32_t, 2> node = {0, 0};
            
                bool identical = true;
                for (uint32_t i = 0; i < 8; ++i) {
                    bool child_is_valid = !is_node_empty(queues.at(d).at(i));
                    bool child_is_leaf = is_node_leaf(queues.at(d).at(i));
                    node[1] |= child_is_valid << (7 - i);
                    node[1] |= (child_is_valid && child_is_leaf) << (15 - i);
                    identical = identical && nodes_equal(queues.at(d).at(i), queues.at(d).at(0));
                }
            
                if (identical) {
                    node = queues.at(d).at(0);
                } else {
                    uint32_t first_child_idx = 0;
                    for (uint32_t i = 0; i < 8; ++i) {
                        auto child = queues.at(d).at(i);
                        if (!is_node_empty(child)) {
                            uint32_t child_idx = push_node_to_buffer(buffer, child);
                            first_child_idx = first_child_idx ? first_child_idx : child_idx;
                        }
                    }
                    node[0] = first_child_idx;
                }
            
                queues.at(d - 1).emplace_back(node);
                queues.at(d).clear();
                --d;
            }

            current_morton = morton;
        }
    }

    return queues.at(0).at(0);
}

static std::vector<uint32_t> df_16_16_16_12_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::vector<uint32_t> df_chunk;
    is_empty = true;
    uint32_t k = 12;
    for (uint32_t g_z = 0; g_z < 16; ++g_z) {
        for (uint32_t g_y = 0; g_y < 16; ++g_y) {
            for (uint32_t g_x = 0; g_x < 16; ++g_x) {
                uint32_t sub_lower_x = lower_x + g_x * 1;
                uint32_t sub_lower_y = lower_y + g_y * 1;
                uint32_t sub_lower_z = lower_z + g_z * 1;
                bool sub_is_empty;
                auto sub_chunk = _construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
                if (sub_is_empty) {
                    df_chunk.push_back(0);
                } else {
                    df_chunk.push_back(push_node_to_buffer(buffer, sub_chunk));
                }
                df_chunk.push_back(1);
                is_empty = is_empty && sub_is_empty;
            }
        }
    }
    for (uint32_t g_z = 0; g_z < 16; ++g_z) {
        for (uint32_t g_y = 0; g_y < 16; ++g_y) {
            for (uint32_t g_x = 0; g_x < 16; ++g_x) {
                uint32_t g_voxel_offset = (g_x + g_y * 16 + g_z * 16 * 16);
                uint32_t min_dist = k;
                for (uint32_t kz = g_z > k ? g_z - k : 0; kz <= g_z + k && kz < 16; ++kz) {
                    for (uint32_t ky = g_y > k ? g_y - k : 0; ky <= g_y + k && ky < 16; ++ky) {
                        for (uint32_t kx = g_x > k ? g_x - k : 0; kx <= g_x + k && kx < 16; ++kx) {
                            size_t k_voxel_offset = kx + ky * 16 + kz * 16 * 16;
                            if (df_chunk[k_voxel_offset * 2] != 0) {
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
                df_chunk[g_voxel_offset * 2 + 1] = (min_dist - 1);
            }
        }
    }
    return df_chunk;
}

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}