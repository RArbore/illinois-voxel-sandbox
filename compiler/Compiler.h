#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

enum class Format {
    Raw,
    DF,
    SVO,
    SVDAG,
};

struct InstantiatedFormat {
    Format format_;
    uint32_t parameters_[4];
};

struct Optimizations {
    bool unroll_sv_;
    bool restart_sv_;
    bool whole_level_dedup_;
    bool df_packing_;
};

std::vector<InstantiatedFormat> parse_format(std::string_view format_string);

std::tuple<uint32_t, uint32_t, uint32_t> calculate_bounds(const std::vector<InstantiatedFormat> &format, uint32_t starting_level = 0);

std::string generate_construction_cpp(const std::vector<InstantiatedFormat> &format, const Optimizations &opt);

std::string generate_intersection_glsl(const std::vector<InstantiatedFormat> &format, const Optimizations &opt);

std::string format_identifier(const std::vector<InstantiatedFormat> &format, uint32_t starting_level = 0);
