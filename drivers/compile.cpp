#include <iostream>
#include <fstream>

#include <compiler/Compiler.h>
#include <utils/Assert.h>

int main([[maybe_unused]] int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a format to parse.");

    auto format = parse_format(argv[1]);
    ASSERT(format.size() > 0, "Failed to parse format.");

    std::ofstream construction_cpp(format_identifier(format) + "_construct.cpp");
    construction_cpp << generate_construction_cpp(format);
    construction_cpp.close();

    std::ofstream intersection_glsl(format_identifier(format) + "_intersect.glsl");
    intersection_glsl << generate_intersection_glsl(format);
    intersection_glsl.close();
}
