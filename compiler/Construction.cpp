#include <sstream>
#include "Compiler.h"

std::string generate_construction_cpp(const std::vector<InstantiatedFormat> &format) {
    std::stringstream ss;

    auto print_format_identifier = [&]() {
	for (std::size_t i = 0; i < format.size(); ++i) {
	    switch (format[i].format_) {
	    case Format::Raw:
		ss << "raw_" << format[i].parameters_[0] << "_" << format[i].parameters_[1] << "_" << format[i].parameters_[2];
		break;
	    case Format::DF:
		ss << "df_" << format[i].parameters_[0] << "_" << format[i].parameters_[1] << "_" << format[i].parameters_[2];
		break;
	    case Format::SVO:
		ss << "svo_" << format[i].parameters_[0];
		break;
	    case Format::SVDAG:
		ss << "svdag_" << format[i].parameters_[0];
		break;
	    };
	    if (i + 1 < format.size()) {
		ss << "_";
	    }
	}
    };
    
    ss << R"(#include <vector>

#include "Voxel.h"

std::vector<uint32_t> )";

    print_format_identifier();

    ss << R"(_construct(Voxelizer &voxelizer) {
)";
    
    return ss.str();
}
