#include <cstring>
#include <fstream>

#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/Voxelize.h>
#include <voxels/VoxelChunkGeneration.h>
#include <utils/Assert.h>

std::vector<uint32_t> raw_24_24_24_raw_16_16_16_construct(Voxelizer &voxelizer);

std::vector<uint32_t> df_24_24_24_8_raw_16_16_16_construct(Voxelizer &voxelizer);

std::vector<uint32_t> svo_9_construct(Voxelizer &voxelizer);

std::vector<uint32_t> svdag_9_construct(Voxelizer &voxelizer);

std::vector<uint32_t> df_12_12_12_12_svo_5_construct(Voxelizer &voxelizer);

std::vector<uint32_t> df_12_12_12_12_df_32_32_32_16_construct(Voxelizer &voxelizer);

void print_svo(const std::vector<uint32_t> &svo, uint32_t node, std::string spacing) {
    std::cout << spacing << "Internal Node @ " << node << "\n";
    std::cout << spacing << "Child Offset: " << svo[node] << "\n";
    std::cout << spacing << "Valid Mask: " << (svo[node + 1] & 0xFF) << "\n";
    std::cout << spacing << "Leaf Mask: " << ((svo[node + 1] >> 8) & 0xFF) << "\n";
    ASSERT((svo[node + 1] & 0xFFFF0000) == 0, "");
    for (uint32_t i = 0, count = 0; i < 8; ++i) {
	if ((svo[node + 1] >> (7 - i)) & 1) {
	    if ((svo[node + 1] >> (15 - i)) & 1) {
		std::cout << spacing << "  Leaf Node @ " << (svo[node] + count * 2) << "\n";
		std::cout << spacing << "  Data: " << svo[svo[svo[node] + count * 2]] << "\n";
	    } else {
		print_svo(svo, svo[node] + count * 2, spacing + "  ");
	    }
	    ++count;
	}
    }
}

void print_svdag(const std::vector<uint32_t> &svdag, uint32_t node, std::string spacing) {
    if (svdag[node + 1] == 0xFFFFFFFF) {
	std::cout << spacing << "  Leaf Node @ " << node << "\n";
	std::cout << spacing << "  Data: @ " << svdag[svdag[node]] << "\n";
    } else {
	std::cout << spacing << "Internal Node @ " << node << "\n";
	std::cout << spacing << "Child Offsets: " << svdag[node] << " " << svdag[node + 1] << " " << svdag[node + 2] << " " << svdag[node + 3] << " " << svdag[node + 4] << " " << svdag[node + 5] << " " << svdag[node + 6] << " " << svdag[node + 7] << " " << "\n";
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
    ASSERT(model_path.ends_with(".obj"), "Must provide a .obj model to convert.");
    ASSERT(argv[2], "Must provide a resolution to voxelize at.");
    ASSERT(argv[3], "Must provide a format to output.");

    auto write = [&](const char *ptr, std::size_t num_bytes) {
	std::cout << "Model size: " << num_bytes << "\n";
	
	model_path = model_path.substr(0, model_path.size() - 4);
	std::ofstream stream(model_path,
			     std::ios::out | std::ios::binary);
	stream.write(ptr, num_bytes);
    };
    
    float res = std::stof(std::string(argv[2]));
    uint32_t chunk_width, chunk_height, chunk_depth;
    auto raw_vox = raw_voxelize_obj(model_path, res, chunk_width, chunk_height, chunk_depth);
    std::vector<std::byte> model;
    if (!strcmp(argv[3], "svdag")) {
	model = convert_raw_to_svdag(raw_vox, chunk_width, chunk_height, chunk_depth, 4);
    } else if (!strcmp(argv[3], "raw")) {
	model = append_metadata_to_raw(raw_vox, chunk_width, chunk_height, chunk_depth);
    } else if (!strcmp(argv[3], "df")) {
	model = convert_raw_color_to_df(raw_vox, chunk_width, chunk_height, chunk_depth, 4);
	model = append_metadata_to_raw(model, chunk_width, chunk_height, chunk_depth);
    } else if (!strcmp(argv[3], "raw_24_24_24_raw_16_16_16")) {
	Voxelizer voxelizer(std::move(raw_vox), chunk_width, chunk_height, chunk_depth);
	auto model = raw_24_24_24_raw_16_16_16_construct(voxelizer);
	write(reinterpret_cast<const char *>(model.data()), model.size() * sizeof(uint32_t));
	return 0;
    } else if (!strcmp(argv[3], "df_24_24_24_8_raw_16_16_16")) {
	Voxelizer voxelizer(std::move(raw_vox), chunk_width, chunk_height, chunk_depth);
	auto model = df_24_24_24_8_raw_16_16_16_construct(voxelizer);
	write(reinterpret_cast<const char *>(model.data()), model.size() * sizeof(uint32_t));
	return 0;
    } else if (!strcmp(argv[3], "svo_9")) {
	Voxelizer voxelizer(std::move(raw_vox), chunk_width, chunk_height, chunk_depth);
	auto model = svo_9_construct(voxelizer);
	write(reinterpret_cast<const char *>(model.data()), model.size() * sizeof(uint32_t));
	return 0;
    } else if (!strcmp(argv[3], "svdag_9")) {
	Voxelizer voxelizer(std::move(raw_vox), chunk_width, chunk_height, chunk_depth);
	auto model = svdag_9_construct(voxelizer);
	write(reinterpret_cast<const char *>(model.data()), model.size() * sizeof(uint32_t));
	return 0;
    } else if (!strcmp(argv[3], "df_12_12_12_12_svo_5")) {
	Voxelizer voxelizer(std::move(raw_vox), chunk_width, chunk_height, chunk_depth);
	auto model = df_12_12_12_12_svo_5_construct(voxelizer);
	write(reinterpret_cast<const char *>(model.data()), model.size() * sizeof(uint32_t));
	return 0;
    } else if (!strcmp(argv[3], "df_12_12_12_12_df_32_32_32_16")) {
	Voxelizer voxelizer(std::move(raw_vox), chunk_width, chunk_height, chunk_depth);
	auto model = df_12_12_12_12_df_32_32_32_16_construct(voxelizer);
	write(reinterpret_cast<const char *>(model.data()), model.size() * sizeof(uint32_t));
	return 0;
    } else {
	ASSERT(false, "Unrecognized model format.");
    }
    write(reinterpret_cast<const char *>(model.data()), model.size());
}
