#include <cstdint>
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

static std::array<uint32_t, 8> svdag_12_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty, std::unordered_map<std::array<uint32_t, 8>, uint32_t> &deduplication_map);

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

std::vector<uint32_t> svdag_12_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer {0};
    bool is_empty;
    std::unordered_map<std::array<uint32_t, 8>, uint32_t> deduplication_map;
    auto root_node = svdag_12_construct_node(voxelizer, buffer, 0, 0, 0, is_empty, deduplication_map);
    buffer.at(0) = push_node_to_buffer(buffer, root_node);
    return buffer;
}

static std::array<uint32_t, 8> svdag_12_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty, std::unordered_map<std::array<uint32_t, 8>, uint32_t> &deduplication_map) {
    uint32_t power_of_two = 12;
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

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}
