#include <sstream>
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
		ss << "std::pair<std::array<2, uint32_t>, bool>";
		break;
	    case Format::SVDAG:
		ss << "std::pair<std::array<8, uint32_t>, bool>";
		break;
	    }
	}
    };
    
    ss << R"(#include <vector>

#include "Voxel.h"

)";

    for (uint32_t i = 0; i < format.size() + 1; ++i) {
	ss << R"(static )";
	print_level_node_type(i);
	ss << R"( )";
	print_format_identifier(i);
	ss << R"(_construct(Voxelizer &voxelizer, std::vector<uint32_t> &buffer, uint32_t lower_x, uint32_t lower_y, uint32_t lower_z);

)";
    }

    ss << R"(std::vector<uint32_t> )";

    print_format_identifier();

    ss << R"(_construct(Voxelizer &voxelizer) {
    std::vector<uint32_t> buffer;

    return buffer;
}
)";
    
    return ss.str();
}
