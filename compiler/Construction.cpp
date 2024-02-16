#include <sstream>

#include <utils/Assert.h>

#include "Compiler.h"

std::string generate_construction_cpp(const std::vector<InstantiatedFormat> &format) {
    std::stringstream ss;

    auto print_format_identifier = [&](uint32_t starting_level = 0) {
    ss << format_identifier(format, starting_level);
    };

    auto print_level_node_type = [&](uint32_t level) {
    if (level >= format.size()) {
        ss << "uint32_t";
    } else {
        switch(format[level].format_) {
        case Format::Raw:
        case Format::DF:
        ss << "std::vector<uint32_t>";
        break;
        case Format::SVO:
        ss << "std::array<uint32_t, 2>";
        break;
        case Format::SVDAG:
        ss << "std::array<uint32_t, 8>";
        break;
        }
    }
    };
    
    ss << R"(#include <cstdint>
#include <vector>
#include <array>
#include <map>

#include <external/libmorton/include/libmorton/morton.h>

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

)";

    for (uint32_t i = 0; i < format.size() + 1; ++i) {
    ss << R"(static )";
    print_level_node_type(i);
    ss << R"( )";
    print_format_identifier(i);
    ss << R"(_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty);

)";
    }

    ss << R"(std::vector<uint32_t> )";

    print_format_identifier();

    ss << R"(_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer {0};
    bool is_empty;
    auto root_node = )";

    print_format_identifier();
    ss << R"(_construct_node(voxelizer, buffer, 0, 0, 0, is_empty))";
    ss << R"(;)";

    ss << R"(
    buffer.at(0) = push_node_to_buffer(buffer, root_node);
    return buffer;
}
)";

    for (uint32_t i = 0; i < format.size(); ++i) {
    ss << R"(
static )";
    print_level_node_type(i);
    ss << R"( )";
    print_format_identifier(i);
    ss << R"(_construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
)";
    auto [sub_w, sub_h, sub_d] = calculate_bounds(format, i + 1);
    auto [inc_w, inc_h, inc_d] = calculate_bounds(format, i);
    auto this_w = inc_w / sub_w, this_h = inc_h / sub_h, this_d = inc_d / sub_d;

    switch (format[i].format_) {
    case Format::Raw:
        ss << R"(    std::vector<uint32_t> raw_chunk;
    is_empty = true;
    for (uint32_t g_z = 0; g_z < )" << this_d << R"(; ++g_z) {
        for (uint32_t g_y = 0; g_y < )" << this_h << R"(; ++g_y) {
            for (uint32_t g_x = 0; g_x < )" << this_w << R"(; ++g_x) {
                uint32_t sub_lower_x = lower_x + g_x * )" << sub_w << R"(;
                uint32_t sub_lower_y = lower_y + g_y * )" << sub_h << R"(;
                uint32_t sub_lower_z = lower_z + g_z * )" << sub_d << R"(;
                bool sub_is_empty;
                auto sub_chunk = )";
        print_format_identifier(i + 1);
        ss << R"(_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
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
)";
        break;
    case Format::DF:
        ss << R"(    std::vector<uint32_t> df_chunk;
    is_empty = true;
    uint32_t k = )" << format[i].parameters_[3] << R"(;
    for (uint32_t g_z = 0; g_z < )" << this_d << R"(; ++g_z) {
        for (uint32_t g_y = 0; g_y < )" << this_h << R"(; ++g_y) {
            for (uint32_t g_x = 0; g_x < )" << this_w << R"(; ++g_x) {
                uint32_t sub_lower_x = lower_x + g_x * )" << sub_w << R"(;
                uint32_t sub_lower_y = lower_y + g_y * )" << sub_h << R"(;
                uint32_t sub_lower_z = lower_z + g_z * )" << sub_d << R"(;
                bool sub_is_empty;
                auto sub_chunk = )";
        print_format_identifier(i + 1);
        ss << R"(_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
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
    for (uint32_t g_z = 0; g_z < )" << this_d << R"(; ++g_z) {
        for (uint32_t g_y = 0; g_y < )" << this_h << R"(; ++g_y) {
            for (uint32_t g_x = 0; g_x < )" << this_w << R"(; ++g_x) {
                uint32_t g_voxel_offset = (g_x + g_y * )" << this_w << R"( + g_z * )" << this_w << R"( * )" << this_h << R"();
            uint32_t min_dist = k;
            for (uint32_t kz = g_z > k ? g_z - k : 0; kz <= g_z + k && kz < )" << this_d << R"(; ++kz) {
                for (uint32_t ky = g_y > k ? g_y - k : 0; ky <= g_y + k && ky < )" << this_h << R"(; ++ky) {
                    for (uint32_t kx = g_x > k ? g_x - k : 0; kx <= g_x + k && kx < )" << this_w << R"(; ++kx) {
                        size_t k_voxel_offset = kx + ky * )" << this_w << R"( + kz * )" << this_w << R"( * )" << this_h << R"(;
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
                df_chunk[g_voxel_offset * 2 + 1] = min_dist;
            }
        }
    }
    return df_chunk;
)";
        break;
    case Format::SVO:
        ss << R"(    uint32_t power_of_two = )" << format[i].parameters_[0] << R"(;
    uint32_t bounded_edge_length = 1 << power_of_two;
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
        uint32_t sub_lower_x = x + lower_x, sub_lower_y = y + lower_y, sub_lower_z = z + lower_z;
        bool sub_is_empty;
        auto sub_chunk = )";
	print_format_identifier(i + 1);
	ss << R"(_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
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
)";
        break;
    case Format::SVDAG:
        ss << R"(    uint32_t power_of_two = )" << format[i].parameters_[0] << R"(;
    uint32_t bounded_edge_length = 1 << power_of_two;
    std::vector<std::vector<std::array<uint32_t, 8>>> queues(power_of_two + 1);
    const uint64_t num_voxels = bounded_edge_length * bounded_edge_length * bounded_edge_length;

    auto nodes_equal = [](const std::array<uint32_t, 8> &a, const std::array<uint32_t, 8> &b) {
        return a == b;
    };

    auto is_node_empty = [](const std::array<uint32_t, 8> &node) {
        return !node[0] && !node[1] && !node[2] && !node[3] && !node[4] && !node[5] && !node[6] && !node[7];
    };

    std::map<std::array<uint32_t, 8>, uint32_t> deduplication_map;

    is_empty = true;

    for (uint64_t morton = 0; morton < num_voxels; ++morton) {
        uint_fast32_t x = 0, y = 0, z = 0;
        libmorton::morton3D_64_decode(morton, x, y, z);

        std::array<uint32_t, 8> node = {0, 0, 0, 0, 0, 0, 0, 0};
        uint32_t sub_lower_x = x + lower_x, sub_lower_y = y + lower_y, sub_lower_z = z + lower_z;
        bool sub_is_empty;
        auto sub_chunk = )";
	print_format_identifier(i + 1);
	ss << R"(_construct_node(voxelizer, buffer, sub_lower_x, sub_lower_y, sub_lower_z, sub_is_empty);
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
)";
        break;
    default:
        ASSERT(false, "Unhandled format.");
    };

    ss << R"(}
)";
    }

    ss << R"(
static uint32_t _construct_node(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z, bool &is_empty) {
    uint32_t voxel = voxelizer.at(lower_x, lower_y, lower_z);
    is_empty = voxel == 0;
    return voxel;
}
)";
    
    return ss.str();
}
