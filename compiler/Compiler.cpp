#include <charconv>
#include <cstring>

#include "Compiler.h"

std::vector<InstantiatedFormat> parse_format(std::string_view format_string) {
    std::vector<InstantiatedFormat> format;

    auto eat = [&](uint32_t num) {
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
	    if (first_non_number == 0) {
		return false;
	    }
	    uint32_t number;
	    if (std::from_chars(format_string.data(), format_string.data() + first_non_number, number).ec != std::errc{}) {
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
	InstantiatedFormat format;
	memset(&format, 0, sizeof(format));

	if (format_string.starts_with("Raw")) {
	    format.format_ = Format::Raw;
	    eat(3);
	    if (!parse_parameters(format.parameters_, 3)) {
		return {};
	    }
	} else if (format_string.starts_with("DF")) {
	    format.format_ = Format::DF;
	    eat(2);
	    if (!parse_parameters(format.parameters_, 3)) {
		return {};
	    }
	} else if (format_string.starts_with("SVO")) {
	    format.format_ = Format::SVO;
	    eat(3);
	    if (!parse_parameters(format.parameters_, 1)) {
		return {};
	    }
	} else if (format_string.starts_with("SVDAG")) {
	    format.format_ = Format::SVDAG;
	    eat(5);
	    if (!parse_parameters(format.parameters_, 1)) {
		return {};
	    }
	} else {
	    return {};
	}

	clear_white_space();
    }

    return format;
}
