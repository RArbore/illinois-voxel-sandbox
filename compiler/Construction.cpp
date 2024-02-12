#include <sstream>

#include <utils/Assert.h>

#include "Compiler.h"

std::string generate_construction_cpp(const std::vector<InstantiatedFormat> &format) {
    std::stringstream ss;

    auto print_level_identifier = [&](uint32_t level) {
	    switch (format[level].format_) {
	    case Format::Raw:
		ss << "raw_" << format[level].parameters_[0] << "_" << format[level].parameters_[1] << "_" << format[level].parameters_[2];
		break;
	    case Format::DF:
		ss << "df_" << format[level].parameters_[0] << "_" << format[level].parameters_[1] << "_" << format[level].parameters_[2];
		break;
	    case Format::SVO:
		ss << "svo_" << format[level].parameters_[0];
		break;
	    case Format::SVDAG:
		ss << "svdag_" << format[level].parameters_[0];
		break;
	    };
    };

    auto print_format_identifier = [&](uint32_t starting_level = 0) {
	for (uint32_t i = starting_level; i < format.size(); ++i) {
	    print_level_identifier(i);
	    if (i + 1 < format.size()) {
		ss << "_";
	    }
	}
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

#include "Voxel.h"

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
    std::vector<uint32_t> buffer;
    bool is_empty;
    auto root_node = )";

    print_format_identifier();
    ss << R"(_construct_node(voxelizer, buffer, 0, 0, 0, is_empty))";
    print_format_node_accessor(0);
    ss << R"(;)";

    ss << R"(
    push_node_to_buffer(buffer, root_node);
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
