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

static std::vector<uint32_t> raw_256_256_256_svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty, std::unordered_map<std::vector<uint32_t>, uint32_t> &deduplication_map);

static uint32_t _construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

void raw_256_256_256_svdag_3_construct(Voxelizer &voxelizer, std::ofstream &buffer) {
    bool is_empty;
    uint32_t size = 0;
    buffer.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    ++size;
    auto root_node = raw_256_256_256_svdag_3_construct_node(voxelizer, {buffer, size}, 0, 0, 0, is_empty);
    uint32_t root = push_node_to_buffer({buffer, size}, root_node);
    buffer.seekp(0, std::ios_base::beg);
    buffer.write(reinterpret_cast<const char *>(&root), sizeof(uint32_t));
    buffer.seekp(0, std::ios_base::end);
}

static std::vector<uint32_t> raw_256_256_256_svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::unordered_map<std::vector<uint32_t>, uint32_t> deduplication_map;

    is_empty = true;
    uint64_t num_voxels = 16777216;
    std::vector<uint32_t> raw_chunk(num_voxels);
    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t g_x = 0, g_y = 0, g_z = 0;
        libmorton::morton3D_64_decode(morton, g_x, g_y, g_z);
        uint64_t linear_idx = g_x + g_y * 256 + g_z * 256 * 256;
        uint32_t sub_lower_x = lower_x + g_x * 8;
        uint32_t sub_lower_y = lower_y + g_y * 8;
        uint32_t sub_lower_z = lower_z + g_z * 8;
        bool sub_is_empty;
        auto sub_chunk = svdag_3_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty, deduplication_map);
        if (!sub_is_empty) {
            raw_chunk.at(linear_idx) = push_node_to_buffer(buffer, sub_chunk);
        }
        is_empty = is_empty && sub_is_empty;
    }
    return raw_chunk;
}

static std::vector<uint32_t> svdag_3_construct_node(Voxelizer &voxelizer, std::pair<std::ofstream &, uint32_t &> buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty, std::unordered_map<std::vector<uint32_t>, uint32_t> &deduplication_map) {
    uint32_t power_of_two = 3;
    const uint64_t bounded_edge_length = 1 << power_of_two;
    std::vector<std::vector<std::vector<uint32_t>>> queues(power_of_two + 1);
    const uint64_t num_voxels = bounded_edge_length * bounded_edge_length * bounded_edge_length;

    auto nodes_equal = [](const std::vector<uint32_t> &a, const std::vector<uint32_t> &b) {
        return a == b;
    };

    auto is_node_empty = [](const std::vector<uint32_t> &node) {
        return !node.at(0);
    };

    auto is_node_leaf = [](const std::vector<uint32_t> &node) {
        return node.at(0) && node.size() == 1;
    };

    is_empty = true;

    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

        std::vector<uint32_t> node = {0};
        uint32_t sub_lower_x = x * 1 + lower_x, sub_lower_y = y * 1 + lower_y, sub_lower_z = z * 1 + lower_z;
        bool sub_is_empty;
        auto sub_chunk = _construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
        if (!sub_is_empty) {
            node[0] = push_node_to_buffer(buffer, sub_chunk);
            is_empty = false;
        }

        queues.at(power_of_two).emplace_back(node);
        uint32_t d = power_of_two;
        while (d > 0 && queues.at(d).size() == 8) {
            std::vector<uint32_t> node = {0};

            bool identical = true;
            for (uint32_t i = 0; i < 8; ++i) {
                bool child_is_valid = !is_node_empty(queues.at(d).at(i));
                bool child_is_leaf = is_node_leaf(queues.at(d).at(i));
                node[0] |= child_is_valid << (7 - i);
                node[0] |= (child_is_valid && child_is_leaf) << (15 - i);
                identical = identical && nodes_equal(queues.at(d).at(i), queues.at(d).at(0));
            }

            if (identical) {
                node = queues.at(d).at(0);
            } else {
                for (uint32_t i = 0; i < 8; ++i) {
                    auto child = queues.at(d).at(i);
                    if (!is_node_empty(child)) {
                        uint32_t child_idx;
                        if (deduplication_map.contains(child)) {
                            child_idx = deduplication_map[child];
                        } else {
                            child_idx = push_node_to_buffer(buffer, child);
                            deduplication_map.emplace(child, child_idx);
                        }
                        node.push_back(child_idx);
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
