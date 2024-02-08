#include <cstring>
#include <fstream>

#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/Voxelize.h>
#include <voxels/VoxelChunkGeneration.h>
#include <utils/Assert.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a .obj model to convert.");
    std::string model_path(argv[1]);
    ASSERT(model_path.ends_with(".obj"), "Must provide a .obj model to convert.");
    ASSERT(argv[2], "Must provide a resolution to voxelize at.");
    ASSERT(argv[3], "Must provide a format to output.");
    float res = std::stof(std::string(argv[2]));
    uint32_t chunk_width, chunk_height, chunk_depth;
    auto raw_vox = raw_voxelize_obj(model_path, res, chunk_width, chunk_height, chunk_depth);
    std::vector<std::byte> model;
    if (!strcmp(argv[3], "svdag")) {
	model = convert_raw_to_svdag(raw_vox, chunk_width, chunk_height, chunk_depth, 4);
    } else if (!strcmp(argv[3], "raw")) {
	model = append_metadata_to_raw(raw_vox, chunk_width, chunk_height, chunk_depth);
    } else {
	ASSERT(false, "Unrecognized model format.");
    }
    std::cout << "Model size: " << model.size() << "\n";

    model_path = model_path.substr(0, model_path.size() - 4);
    std::ofstream stream(model_path,
			 std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<char*>(model.data()), model.size());
}
