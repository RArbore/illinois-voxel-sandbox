#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

enum class Format {
    Raw,
    DF,
    SVO,
    SVDAG,
};

struct InstantiatedFormat {
    Format format_;
    uint32_t parameters_[3];
};

std::vector<InstantiatedFormat> parse_format(std::string_view format_string);

std::tuple<uint32_t, uint32_t, uint32_t> calculate_bounds(const std::vector<InstantiatedFormat> &format);

std::string generate_construction_cpp(const std::vector<InstantiatedFormat> &format);
