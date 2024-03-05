#include <iostream>
#include <cstring>
#include <fstream>

#include <compiler/Compiler.h>
#include <utils/Assert.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a format to parse.");

    auto format = parse_format(argv[1]);
    ASSERT(format.size() > 0, "Failed to parse format.");

    Optimizations opts { false, false, false };
    for (int i = 2; i < argc; ++i) {
	if (!strcmp(argv[i], "-unroll-sv") || !strcmp(argv[i], "--unroll-sv")) {
	    opts.unroll_sv_ = true;
	} else if (!strcmp(argv[i], "-whole-level-dedup") || !strcmp(argv[i], "--whole-level-dedup")) {
	    opts.whole_level_dedup_ = true;
	} else if (!strcmp(argv[i], "-df-packing") || !strcmp(argv[i], "--df-packing")) {
	    opts.df_packing_ = true;
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
