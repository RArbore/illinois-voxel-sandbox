#include <iostream>
#include <cstring>
#include <fstream>

#include <compiler/Compiler.h>
#include <utils/Assert.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a format to parse.");

    auto format = parse_format(argv[1]);
    ASSERT(format.size() > 0, "Failed to parse format.");

    auto format_iden = format_identifier(format);

    Optimizations opts { false, false, false, false };
    for (int i = 2; i < argc; ++i) {
	if (!strcmp(argv[i], "-just-get-iden") || !strcmp(argv[i], "--just-get-iden")) {
	    std::cout << format_iden;
	    return 0;
	} else if (!strcmp(argv[i], "-unroll-sv") || !strcmp(argv[i], "--unroll-sv")) {
	    opts.unroll_sv_ = true;
	} else if (!strcmp(argv[i], "-restart-sv") || !strcmp(argv[i], "--restart-sv")) {
	    opts.restart_sv_ = true;
	} else if (!strcmp(argv[i], "-whole-level-dedup") || !strcmp(argv[i], "--whole-level-dedup")) {
	    opts.whole_level_dedup_ = true;
	} else if (!strcmp(argv[i], "-df-packing") || !strcmp(argv[i], "--df-packing")) {
	    opts.df_packing_ = true;
	} else {
	    ASSERT(false, "Failed to parse optimization flag.");
	}
    }
    ASSERT(!opts.unroll_sv_ || !opts.restart_sv_, "Cannot enable both unrolling sparse voxel intersection and restart sparse voxel intersection.");

    std::ofstream construction_cpp(format_iden +
                                   "_construct.cpp");
    construction_cpp << generate_construction_cpp(format, opts);
    construction_cpp.close();

    std::ofstream intersection_glsl(format_iden +
                                    "_intersect.glsl");
    intersection_glsl << generate_intersection_glsl(format, opts);
    intersection_glsl.close();

    std::cout << format_iden;
}
