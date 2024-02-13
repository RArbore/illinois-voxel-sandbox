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
		ss << "std::pair<std::array<uint32_t, 2>, bool>";
		break;
	    case Format::SVDAG:
		ss << "std::pair<std::array<uint32_t, 8>, bool>";
		break;
	    }
	}
    };

    auto print_format_node_accessor = [&](uint32_t level = 0) {
	if (level < format.size()) {
	    switch(format[level].format_) {
	    case Format::SVO:
	    case Format::SVDAG:
		ss << ".first";
		break;
	    default:
		break;
	    }
	}
    };
    
    ss << R"(#include <cstdint>
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
    print_format_node_accessor(0);
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
    for (uint32_t g_x = 0; g_x < )" << this_w << R"(; ++g_x) {
        for (uint32_t g_y = 0; g_y < )" << this_h << R"(; ++g_y) {
            for (uint32_t g_z = 0; g_z < )" << this_d << R"(; ++g_z) {
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
                    raw_chunk.push_back(push_node_to_buffer(buffer, sub_chunk)";
	    print_format_node_accessor(i + 1);
	    ss << R"());
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
    for (uint32_t g_x = 0; g_x < )" << this_w << R"(; ++g_x) {
        for (uint32_t g_y = 0; g_y < )" << this_h << R"(; ++g_y) {
            for (uint32_t g_z = 0; g_z < )" << this_d << R"(; ++g_z) {
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
                    df_chunk.push_back(push_node_to_buffer(buffer, sub_chunk)";
	    print_format_node_accessor(i + 1);
	    ss << R"());
                }
                df_chunk.push_back(k);
                is_empty = is_empty && sub_is_empty;
            }
        }
    }
    if (!is_empty) {
        for (uint32_t g_x = 0; g_x < )" << this_w << R"(; ++g_x) {
            for (uint32_t g_y = 0; g_y < )" << this_h << R"(; ++g_y) {
                for (uint32_t g_z = 0; g_z < )" << this_d << R"(; ++g_z) {
                    uint32_t g_voxel_offset = (g_x + g_y * )" << this_w << R"( + g_z * )" << this_w << R"( * )" << this_h << R"();
                    if (df_chunk[g_voxel_offset * 2]) {
                        uint32_t limit_nx = 0, limit_ny = 0, limit_nz = 0, limit_px = 0, limit_py = 0, limit_pz = 0;
                        for (uint32_t d = 1; d < k && !(limit_nx && limit_ny && limit_nz && limit_px && limit_py && limit_pz); ++d) {
                            uint32_t bound_nx = limit_nx ? limit_nx : d > g_x ? 0 : g_x - d;
                            uint32_t bound_ny = limit_ny ? limit_ny : d > g_y ? 0 : g_y - d;
                            uint32_t bound_nz = limit_nz ? limit_nz : d > g_z ? 0 : g_z - d;
                            uint32_t bound_px = limit_px ? limit_px : d + g_x < )" << this_w << R"( ? )" << (this_w - 1) << R"( : g_x + d;
                            uint32_t bound_py = limit_py ? limit_py : d + g_y < )" << this_h << R"( ? )" << (this_h - 1) << R"( : g_y + d;
                            uint32_t bound_pz = limit_pz ? limit_pz : d + g_z < )" << this_d << R"( ? )" << (this_d - 1) << R"( : g_z + d;
                            for (uint32_t k_x = bound_nx; k_x <= bound_px; ++k_x) {
                                for (uint32_t k_y = bound_ny; k_y <= bound_py; ++k_y) {
                                    uint32_t k_n_voxel_offset = (k_x + k_y * )" << this_w << R"( + bound_nz * )" << this_w << R"( * )" << this_h << R"();
                                    uint32_t prev_n = df_chunk[k_n_voxel_offset * 2 + 1];
                                    if (df_chunk[k_n_voxel_offset * 2]) {
                                        limit_nz = bound_nz;
                                    }
                                    df_chunk[k_n_voxel_offset * 2 + 1] = prev_n > d ? d : prev_n;

                                    uint32_t k_p_voxel_offset = (k_x + k_y * )" << this_w << R"( + bound_pz * )" << this_w << R"( * )" << this_h << R"();
                                    uint32_t prev_p = df_chunk[k_p_voxel_offset * 2 + 1];
                                    if (df_chunk[k_p_voxel_offset * 2]) {
                                        limit_pz = bound_pz;
                                    }
                                    df_chunk[k_p_voxel_offset * 2 + 1] = prev_p > d ? d : prev_p;
                                }
                            }
                            for (uint32_t k_x = bound_nx; k_x <= bound_px; ++k_x) {
                                for (uint32_t k_z = bound_nz; k_z <= bound_pz; ++k_z) {
                                    uint32_t k_n_voxel_offset = (k_x + bound_ny * )" << this_w << R"( + k_z * )" << this_w << R"( * )" << this_h << R"();
                                    uint32_t prev_n = df_chunk[k_n_voxel_offset * 2 + 1];
                                    if (df_chunk[k_n_voxel_offset * 2]) {
                                        limit_ny = bound_ny;
                                    }
                                    df_chunk[k_n_voxel_offset * 2 + 1] = prev_n > d ? d : prev_n;

                                    uint32_t k_p_voxel_offset = (k_x + bound_py * )" << this_w << R"( + k_z * )" << this_w << R"( * )" << this_h << R"();
                                    uint32_t prev_p = df_chunk[k_p_voxel_offset * 2 + 1];
                                    if (df_chunk[k_p_voxel_offset * 2]) {
                                        limit_py = bound_py;
                                    }
                                    df_chunk[k_p_voxel_offset * 2 + 1] = prev_p > d ? d : prev_p;
                                }
                            }
                            for (uint32_t k_y = bound_ny; k_y <= bound_py; ++k_y) {
                                for (uint32_t k_z = bound_nz; k_z <= bound_pz; ++k_z) {
                                    uint32_t k_n_voxel_offset = (bound_nx + k_y * )" << this_w << R"( + k_z * )" << this_w << R"( * )" << this_h << R"();
                                    uint32_t prev_n = df_chunk[k_n_voxel_offset * 2 + 1];
                                    if (df_chunk[k_n_voxel_offset * 2]) {
                                        limit_nx = bound_nx;
                                    }
                                    df_chunk[k_n_voxel_offset * 2 + 1] = prev_n > d ? d : prev_n;

                                    uint32_t k_p_voxel_offset = (bound_px + k_y * )" << this_w << R"( + k_z * )" << this_w << R"( * )" << this_h << R"();
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
