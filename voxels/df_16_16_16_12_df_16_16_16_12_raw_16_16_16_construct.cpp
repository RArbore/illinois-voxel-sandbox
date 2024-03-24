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

static std::vector<uint32_t> df_16_16_16_12_df_16_16_16_12_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> df_16_16_16_12_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

std::vector<uint32_t> df_16_16_16_12_df_16_16_16_12_raw_16_16_16_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer {0};
    bool is_empty;
    auto root_node = df_16_16_16_12_df_16_16_16_12_raw_16_16_16_construct_node(voxelizer, buffer, 0, 0, 0, is_empty);
    buffer.at(0) = push_node_to_buffer(buffer, root_node);
    return buffer;
}

static std::vector<uint32_t> df_16_16_16_12_df_16_16_16_12_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::vector<uint32_t> df_chunk;
    is_empty = true;
    uint32_t k = 12;
    for (uint32_t g_z = 0; g_z < 16; ++g_z) {
        for (uint32_t g_y = 0; g_y < 16; ++g_y) {
            for (uint32_t g_x = 0; g_x < 16; ++g_x) {
                uint32_t sub_lower_x = lower_x + g_x * 256;
                uint32_t sub_lower_y = lower_y + g_y * 256;
                uint32_t sub_lower_z = lower_z + g_z * 256;
                bool sub_is_empty;
                auto sub_chunk = df_16_16_16_12_raw_16_16_16_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
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

static std::vector<uint32_t> df_16_16_16_12_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::vector<uint32_t> df_chunk;
    is_empty = true;
    uint32_t k = 12;
    for (uint32_t g_z = 0; g_z < 16; ++g_z) {
        for (uint32_t g_y = 0; g_y < 16; ++g_y) {
            for (uint32_t g_x = 0; g_x < 16; ++g_x) {
                uint32_t sub_lower_x = lower_x + g_x * 16;
                uint32_t sub_lower_y = lower_y + g_y * 16;
                uint32_t sub_lower_z = lower_z + g_z * 16;
                bool sub_is_empty;
                auto sub_chunk = raw_16_16_16_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
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

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::vector<uint32_t> raw_chunk;
    is_empty = true;
    for (uint32_t g_z = 0; g_z < 16; ++g_z) {
        for (uint32_t g_y = 0; g_y < 16; ++g_y) {
            for (uint32_t g_x = 0; g_x < 16; ++g_x) {
                uint32_t sub_lower_x = lower_x + g_x * 1;
                uint32_t sub_lower_y = lower_y + g_y * 1;
                uint32_t sub_lower_z = lower_z + g_z * 1;
                bool sub_is_empty;
                auto sub_chunk = _construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
                if (sub_is_empty) {
                    raw_chunk.push_back(0);
                } else {
                    raw_chunk.push_back(push_node_to_buffer(buffer, sub_chunk));
                }
                is_empty = is_empty && sub_is_empty;
            }
        }
    }
    return raw_chunk;
}

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}
