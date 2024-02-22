#include <iostream>
#include <cstring>
#include <fstream>

#include <compiler/Compiler.h>
#include <utils/Assert.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a format to parse.");

    auto format = parse_format(argv[1]);
    ASSERT(format.size() > 0, "Failed to parse format.");

    Optimizations opts { false, false };
    for (int i = 2; i < argc; ++i) {
	if (!strcmp(argv[i], "-unroll_sv") || !strcmp(argv[i], "--unroll_sv")) {
	    opts.unroll_sv_ = true;
	} else if (!strcmp(argv[i], "-whole_level_dedup") || !strcmp(argv[i], "--whole_level_dedup")) {
	    opts.whole_level_dedup_ = true;
	} else {
	    ASSERT(false, "Failed to parse optimization flag.");
	}
    }

    std::ofstream construction_cpp(format_identifier(format) +
                                   "_construct.cpp");
    construction_cpp << generate_construction_cpp(format, opts);
    construction_cpp.close();

    std::ofstream intersection_glsl(format_identifier(format) +
                                    "_intersect.glsl");
    intersection_glsl << generate_intersection_glsl(format, opts);
    intersection_glsl.close();
}
