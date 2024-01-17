#include <fstream>

#include <graphics/GraphicsContext.h>
#include <utils/Assert.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include <voxels/Voxelize.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a .obj model to convert.");
    std::string model_path(argv[1]);
    ASSERT(model_path.ends_with(".obj"),
           "Must provide a .obj model to convert.");
    ASSERT(argv[2], "Must provide a resolution to voxelize at.");
    float res = std::stof(std::string(argv[2]));
    uint32_t chunk_width, chunk_height, chunk_depth;
    auto raw_vox = raw_voxelize_obj(model_path, res, chunk_width, chunk_height,
                                    chunk_depth);
    auto svdag = convert_raw_to_svdag(raw_vox, chunk_width, chunk_height,
                                      chunk_depth, 4);
    std::cout << "SVDAG Size: " << svdag.size() << "\n";
    model_path = model_path.substr(0, model_path.size() - 4);
    std::ofstream stream(model_path, std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<char *>(svdag.data()), svdag.size());
}
