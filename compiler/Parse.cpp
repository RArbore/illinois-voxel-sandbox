#include <charconv>
#include <cstring>

#include "Compiler.h"

std::vector<InstantiatedFormat> parse_format(std::string_view format_string) {
    std::vector<InstantiatedFormat> format;

    auto eat = [&](std::size_t num) {
	if (num == std::string::npos) {
	    return;
	}
	format_string = format_string.substr(num);
    };

    auto clear_white_space = [&]() {
	eat(format_string.find_first_not_of(" \n\t\r"));
    };

    auto parse_parameters = [&](uint32_t *dst, uint32_t num) -> bool {
	if (!format_string.starts_with("(")) {
	    return false;
	}
	eat(1);

	clear_white_space();
	for (uint32_t i = 0; i < num; ++i) {
	    auto first_non_number = format_string.find_first_not_of("0123456789");
	    if (first_non_number == 0 || first_non_number == std::string::npos) {
		return false;
	    }
	    uint32_t number;
	    if (std::from_chars(format_string.data(), format_string.data() + first_non_number, number).ec != std::errc{} || number == 0) {
		return false;
	    }
	    dst[i] = number;
	    eat(first_non_number);

	    if (i + 1 < num) {
		clear_white_space();
		if (!format_string.starts_with(",")) {
		    return false;
		}
		eat(1);
	    }

	    clear_white_space();
	}

	if (!format_string.starts_with(")")) {
	    return false;
	}
	eat(1);

	return true;
    };

    clear_white_space();
    while (!format_string.empty()) {
	InstantiatedFormat single_format;
	memset(&single_format, 0, sizeof(single_format));

	if (format_string.starts_with("Raw")) {
	    single_format.format_ = Format::Raw;
	    eat(3);
	    if (!parse_parameters(single_format.parameters_, 3)) {
		return {};
	    }
	} else if (format_string.starts_with("DF")) {
	    single_format.format_ = Format::DF;
	    eat(2);
	    if (!parse_parameters(single_format.parameters_, 3)) {
		return {};
	    }
	} else if (format_string.starts_with("SVO")) {
	    single_format.format_ = Format::SVO;
	    eat(3);
	    if (!parse_parameters(single_format.parameters_, 1)) {
		return {};
	    }
	} else if (format_string.starts_with("SVDAG")) {
	    single_format.format_ = Format::SVDAG;
	    eat(5);
	    if (!parse_parameters(single_format.parameters_, 1)) {
		return {};
	    }
	} else {
	    return {};
	}

	clear_white_space();
	format.push_back(single_format);
    }

    return format;
}

std::tuple<uint32_t, uint32_t, uint32_t> calculate_bounds(const std::vector<InstantiatedFormat> &format, uint32_t starting_level) {
    uint32_t running_w = 1, running_h = 1, running_d = 1;
    for (uint32_t i = starting_level; i < format.size(); ++i) {
	switch (format[i].format_) {
	case Format::Raw:
	case Format::DF:
	    running_w *= format[i].parameters_[0];
	    running_h *= format[i].parameters_[1];
	    running_d *= format[i].parameters_[2];
	    break;
	case Format::SVO:
	case Format::SVDAG:
	    running_w *= 1 << (format[i].parameters_[0] - 1);
	    running_h *= 1 << (format[i].parameters_[0] - 1);
	    running_d *= 1 << (format[i].parameters_[0] - 1);
	    break;
	};
    }
    return {running_w, running_h, running_d};
}
