#include <iostream>

#include <compiler/Compiler.h>
#include <utils/Assert.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a format to parse.");

    auto format = parse_format(argv[1]);
    ASSERT(format.size() > 0, "Failed to parse format.");
    
    for (const auto &level : format) {
	switch (level.format_) {
	case Format::Raw:
	    std::cout << "Raw(" << level.parameters_[0] << ", " << level.parameters_[1] << ", " << level.parameters_[2] << ")\n";
	    break;
	case Format::DF:
	    std::cout << "DF(" << level.parameters_[0] << ", " << level.parameters_[1] << ", " << level.parameters_[2] << ")\n";
	    break;
	case Format::SVO:
	    std::cout << "SVO(" << level.parameters_[0] << ")\n";
	    break;
	case Format::SVDAG:
	    std::cout << "SVDAG(" << level.parameters_[0] << ")\n";
	    break;
	};
    }

    auto [w, h, d] = calculate_bounds(format);
    std::cout << w << " x " << h << " x " << d << "\n";

    std::cout << generate_construction_cpp(format);
}
