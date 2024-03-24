#include <cstring>
#include <fstream>

#include <compiler/Compiler.h>
#include <graphics/GraphicsContext.h>
#include <utils/Assert.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include <voxels/Voxelize.h>

std::vector<uint32_t> raw_24_24_24_raw_16_16_16_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_24_24_24_8_raw_16_16_16_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svo_9_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svdag_9_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_12_12_12_12_svo_5_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_12_12_12_12_svdag_5_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_15_7_9_8_svdag_7_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svdag_11_construct(Voxelizer &voxelizer);

std::vector<uint32_t> df_16_16_16_12_df_16_16_16_12_df_16_16_16_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_16_16_16_12_df_16_16_16_12_raw_16_16_16_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_16_16_16_12_raw_16_16_16_df_16_16_16_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_256_256_256_12_df_16_16_16_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_256_256_256_12_svdag_4_construct(Voxelizer &voxelizer);
std::vector<uint32_t> df_256_256_256_12_svo_4_construct(Voxelizer &voxelizer);
std::vector<uint32_t> raw_16_16_16_df_16_16_16_12_df_16_16_16_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> raw_16_16_16_raw_16_16_16_raw_16_16_16_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svdag_8_df_16_16_16_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svdag_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svo_8_df_16_16_16_12_construct(Voxelizer &voxelizer);
std::vector<uint32_t> svo_12_construct(Voxelizer &voxelizer);

static const std::unordered_map<std::string, std::vector<uint32_t> (*)(Voxelizer &)> format_to_conversion_function = {
    {"Raw(24, 24, 24) Raw(16, 16, 16)", raw_24_24_24_raw_16_16_16_construct},
    {"DF(24, 24, 24) Raw(16, 16, 16)", df_24_24_24_8_raw_16_16_16_construct},
    {"SVO(9)", svo_9_construct},
    {"SVDAG(9)", svdag_9_construct},
    {"DF(12, 12, 12, 12) SVO(5)", df_12_12_12_12_svo_5_construct},
    {"DF(12, 12, 12, 12) SVDAG(5)", df_12_12_12_12_svdag_5_construct},
    {"DF(15, 7, 9, 8) SVDAG(7)", df_15_7_9_8_svdag_7_construct},
    {"SVDAG(11)", svdag_11_construct},
    {"Raw(16, 16, 16) Raw(16, 16, 16) Raw(16, 16, 16)", df_16_16_16_12_df_16_16_16_12_df_16_16_16_12_construct},

    {"Raw(16, 16, 16) DF(16, 16, 16, 12) DF(16, 16, 16, 12)", df_16_16_16_12_df_16_16_16_12_raw_16_16_16_construct},
    {"DF(16, 16, 16, 12) Raw(16, 16, 16) DF(16, 16, 16, 12)", df_16_16_16_12_raw_16_16_16_df_16_16_16_12_construct},
    {"DF(16, 16, 16, 12) DF(16, 16, 16, 12) Raw(16, 16, 16)", df_256_256_256_12_df_16_16_16_12_construct},
    {"DF(16, 16, 16, 12) DF(16, 16, 16, 12) DF(16, 16, 16, 12)", df_256_256_256_12_svdag_4_construct},
    {"SVO(8) DF(16, 16, 16, 12)", df_256_256_256_12_svo_4_construct},
    {"SVDAG(8) DF(16, 16, 16, 12)", raw_16_16_16_df_16_16_16_12_df_16_16_16_12_construct},
    {"DF(256, 256, 256, 12) SVO(4)", raw_16_16_16_raw_16_16_16_raw_16_16_16_construct},
    {"DF(256, 256, 256, 12) SVDAG(4)", svdag_8_df_16_16_16_12_construct},
    {"DF(256, 256, 256, 12) DF(16, 16, 16, 12)", svdag_12_construct},
    {"SVO(12)", svo_8_df_16_16_16_12_construct},
    {"SVDAG(12)", svo_12_construct}
};

void print_svo(const std::vector<uint32_t> &svo, uint32_t node,
               std::string spacing) {
    std::cout << spacing << "Internal Node @ " << node << "\n";
    std::cout << spacing << "Child Offset: " << svo[node] << "\n";
    std::cout << spacing << "Valid Mask: " << (svo[node + 1] & 0xFF) << "\n";
    std::cout << spacing << "Leaf Mask: " << ((svo[node + 1] >> 8) & 0xFF)
              << "\n";
    ASSERT((svo[node + 1] & 0xFFFF0000) == 0, "");
    for (uint32_t i = 0, count = 0; i < 8; ++i) {
        if ((svo[node + 1] >> (7 - i)) & 1) {
            if ((svo[node + 1] >> (15 - i)) & 1) {
                std::cout << spacing << "  Leaf Node @ "
                          << (svo[node] + count * 2) << "\n";
                std::cout << spacing
                          << "  Data: " << svo[svo[svo[node] + count * 2]]
                          << "\n";
            } else {
                print_svo(svo, svo[node] + count * 2, spacing + "  ");
            }
            ++count;
        }
    }
}

void print_svdag(const std::vector<uint32_t> &svdag, uint32_t node,
                 std::string spacing) {
    if (svdag[node + 1] == 0xFFFFFFFF) {
        std::cout << spacing << "  Leaf Node @ " << node << "\n";
        std::cout << spacing << "  Data: @ " << svdag[svdag[node]] << "\n";
    } else {
        std::cout << spacing << "Internal Node @ " << node << "\n";
        std::cout << spacing << "Child Offsets: " << svdag[node] << " "
                  << svdag[node + 1] << " " << svdag[node + 2] << " "
                  << svdag[node + 3] << " " << svdag[node + 4] << " "
                  << svdag[node + 5] << " " << svdag[node + 6] << " "
                  << svdag[node + 7] << " "
                  << "\n";
        for (uint32_t i = 0; i < 8; ++i) {
            if (svdag[node + i]) {
                print_svdag(svdag, svdag[node + i], spacing + "  ");
            }
        }
    }
}

void print_svo(const std::vector<uint32_t> &svo) {
    std::cout << std::hex;
    print_svo(svo, svo[0], "");
    std::cout << std::dec;
}

void print_svdag(const std::vector<uint32_t> &svdag) {
    std::cout << std::hex;
    print_svdag(svdag, svdag[0], "");
    std::cout << std::dec;
}

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a .obj model to convert.");
    std::string model_path(argv[1]);
    ASSERT(model_path.ends_with(".obj"),
           "Must provide a .obj model to convert.");
    ASSERT(argv[2], "Must provide a resolution to voxelize at.");
    ASSERT(argv[3], "Must provide a format to output.");

    auto write = [&](const char *ptr, std::size_t num_bytes) {
        std::cout << "Model size: " << num_bytes << "\n";

        model_path = model_path.substr(0, model_path.size() - 4);
        std::ofstream stream(model_path, std::ios::out | std::ios::binary);
        stream.write(ptr, num_bytes);
    };

    float res = std::stof(std::string(argv[2]));

    std::vector<std::byte> model;
    std::cout << "Converting model to "
              << argv[3] << std::endl;
    if (!strcmp(argv[3], "svdag")) {
        uint32_t chunk_width, chunk_height, chunk_depth;
        auto raw_vox = raw_voxelize_obj(model_path, res, chunk_width,
                                        chunk_height, chunk_depth);
        model = convert_raw_to_svdag(raw_vox, chunk_width, chunk_height,
                                     chunk_depth, 4);
    } else if (!strcmp(argv[3], "raw")) {
        uint32_t chunk_width, chunk_height, chunk_depth;
        auto raw_vox = raw_voxelize_obj(model_path, res, chunk_width,
                                        chunk_height, chunk_depth);
        model = append_metadata_to_raw(raw_vox, chunk_width, chunk_height,
                                       chunk_depth);
    } else if (!strcmp(argv[3], "df")) {
        uint32_t chunk_width, chunk_height, chunk_depth;
        auto raw_vox = raw_voxelize_obj(model_path, res, chunk_width,
                                        chunk_height, chunk_depth);
        model = convert_raw_color_to_df(raw_vox, chunk_width, chunk_height,
                                        chunk_depth, 4);
        model = append_metadata_to_raw(model, chunk_width, chunk_height,
                                       chunk_depth);
    } else {
	std::string format(argv[3]);
        auto bounds = calculate_bounds(parse_format(format));
        Voxelizer voxelizer(model_path, bounds);
        auto model = format_to_conversion_function.at(format)(voxelizer);
        write(reinterpret_cast<const char *>(model.data()),
              model.size() * sizeof(uint32_t));
        return 0;
    }
    write(reinterpret_cast<const char *>(model.data()), model.size());
}
