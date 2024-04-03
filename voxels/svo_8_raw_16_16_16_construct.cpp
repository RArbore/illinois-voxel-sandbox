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

static std::array<uint32_t, 2> svo_8_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

void svo_8_raw_16_16_16_construct(Voxelizer &voxelizer, std::ofstream &buffer) {
    bool is_empty;
    uint32_t size = 0;
    buffer.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    ++size;
    auto root_node = svo_8_raw_16_16_16_construct_node(voxelizer, {buffer, size}, 0, 0, 0, is_empty);
    uint32_t root = push_node_to_buffer({buffer, size}, root_node);
    buffer.seekp(0, std::ios_base::beg);
    buffer.write(reinterpret_cast<const char *>(&root), sizeof(uint32_t));
    buffer.seekp(0, std::ios_base::end);
}

static std::array<uint32_t, 2> svo_8_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
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

    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

        std::array<uint32_t, 2> node = {0, 0};
        uint32_t sub_lower_x = x * 16 + lower_x, sub_lower_y = y * 16 + lower_y, sub_lower_z = z * 16 + lower_z;
        bool sub_is_empty;
        auto sub_chunk = raw_16_16_16_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            node[0] = push_node_to_buffer(buffer, sub_chunk);
            is_empty = false;
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
    }

    return queues.at(0).at(0);
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
