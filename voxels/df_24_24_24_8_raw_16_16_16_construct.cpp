#include <cstdint>
#include <vector>
#include <array>

#include "Voxelize.h"

static uint32_t push_node_to_buffer(std::vector<uint32_t> &buffer, uint32_t node) {
    uint32_t offset = buffer.size();
    buffer.push_back(node);
    return offset;
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

static std::vector<uint32_t> df_24_24_24_8_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

std::vector<uint32_t> df_24_24_24_8_raw_16_16_16_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer {0};
    bool is_empty;
    auto root_node = df_24_24_24_8_raw_16_16_16_construct_node(voxelizer, buffer, 0, 0, 0, is_empty);
    buffer.at(0) = push_node_to_buffer(buffer, root_node);
    return buffer;
}

static std::vector<uint32_t> df_24_24_24_8_raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::vector<uint32_t> df_chunk;
    is_empty = true;
    uint32_t k = 8;
    for (uint32_t g_x = 0; g_x < 24; ++g_x) {
        for (uint32_t g_y = 0; g_y < 24; ++g_y) {
            for (uint32_t g_z = 0; g_z < 24; ++g_z) {
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
                df_chunk.push_back(k);
                is_empty = is_empty && sub_is_empty;
            }
        }
    }
    if (!is_empty) {
        for (uint32_t g_x = 0; g_x < 24; ++g_x) {
            for (uint32_t g_y = 0; g_y < 24; ++g_y) {
                for (uint32_t g_z = 0; g_z < 24; ++g_z) {
                    uint32_t g_voxel_offset = (g_x + g_y * 24 + g_z * 24 * 24);
                    if (df_chunk[g_voxel_offset * 2]) {
                        uint32_t limit_nx = 0, limit_ny = 0, limit_nz = 0, limit_px = 0, limit_py = 0, limit_pz = 0;
                        for (uint32_t d = 1; d < k && !(limit_nx && limit_ny && limit_nz && limit_px && limit_py && limit_pz); ++d) {
                            uint32_t bound_nx = limit_nx ? limit_nx : d > g_x ? 0 : g_x - d;
                            uint32_t bound_ny = limit_ny ? limit_ny : d > g_y ? 0 : g_y - d;
                            uint32_t bound_nz = limit_nz ? limit_nz : d > g_z ? 0 : g_z - d;
                            uint32_t bound_px = limit_px ? limit_px : d + g_x < 24 ? 23 : g_x + d;
                            uint32_t bound_py = limit_py ? limit_py : d + g_y < 24 ? 23 : g_y + d;
                            uint32_t bound_pz = limit_pz ? limit_pz : d + g_z < 24 ? 23 : g_z + d;
                            for (uint32_t k_x = bound_nx; k_x <= bound_px; ++k_x) {
                                for (uint32_t k_y = bound_ny; k_y <= bound_py; ++k_y) {
                                    uint32_t k_n_voxel_offset = (k_x + k_y * 24 + bound_nz * 24 * 24);
                                    uint32_t prev_n = df_chunk[k_n_voxel_offset * 2 + 1];
                                    if (df_chunk[k_n_voxel_offset * 2]) {
                                        limit_nz = bound_nz;
                                    }
                                    df_chunk[k_n_voxel_offset * 2 + 1] = prev_n > d ? d : prev_n;

                                    uint32_t k_p_voxel_offset = (k_x + k_y * 24 + bound_pz * 24 * 24);
                                    uint32_t prev_p = df_chunk[k_p_voxel_offset * 2 + 1];
                                    if (df_chunk[k_p_voxel_offset * 2]) {
                                        limit_pz = bound_pz;
                                    }
                                    df_chunk[k_p_voxel_offset * 2 + 1] = prev_p > d ? d : prev_p;
                                }
                            }
                            for (uint32_t k_x = bound_nx; k_x <= bound_px; ++k_x) {
                                for (uint32_t k_z = bound_nz; k_z <= bound_pz; ++k_z) {
                                    uint32_t k_n_voxel_offset = (k_x + bound_ny * 24 + k_z * 24 * 24);
                                    uint32_t prev_n = df_chunk[k_n_voxel_offset * 2 + 1];
                                    if (df_chunk[k_n_voxel_offset * 2]) {
                                        limit_ny = bound_ny;
                                    }
                                    df_chunk[k_n_voxel_offset * 2 + 1] = prev_n > d ? d : prev_n;

                                    uint32_t k_p_voxel_offset = (k_x + bound_py * 24 + k_z * 24 * 24);
                                    uint32_t prev_p = df_chunk[k_p_voxel_offset * 2 + 1];
                                    if (df_chunk[k_p_voxel_offset * 2]) {
                                        limit_py = bound_py;
                                    }
                                    df_chunk[k_p_voxel_offset * 2 + 1] = prev_p > d ? d : prev_p;
                                }
                            }
                            for (uint32_t k_y = bound_ny; k_y <= bound_py; ++k_y) {
                                for (uint32_t k_z = bound_nz; k_z <= bound_pz; ++k_z) {
                                    uint32_t k_n_voxel_offset = (bound_nx + k_y * 24 + k_z * 24 * 24);
                                    uint32_t prev_n = df_chunk[k_n_voxel_offset * 2 + 1];
                                    if (df_chunk[k_n_voxel_offset * 2]) {
                                        limit_nx = bound_nx;
                                    }
                                    df_chunk[k_n_voxel_offset * 2 + 1] = prev_n > d ? d : prev_n;

                                    uint32_t k_p_voxel_offset = (bound_px + k_y * 24 + k_z * 24 * 24);
                                    uint32_t prev_p = df_chunk[k_p_voxel_offset * 2 + 1];
                                    if (df_chunk[k_p_voxel_offset * 2]) {
                                        limit_px = bound_px;
                                    }
                                    df_chunk[k_p_voxel_offset * 2 + 1] = prev_p > d ? d : prev_p;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return df_chunk;
}

static std::vector<uint32_t> raw_16_16_16_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    std::vector<uint32_t> raw_chunk;
    is_empty = true;
    for (uint32_t g_x = 0; g_x < 16; ++g_x) {
        for (uint32_t g_y = 0; g_y < 16; ++g_y) {
            for (uint32_t g_z = 0; g_z < 16; ++g_z) {
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
