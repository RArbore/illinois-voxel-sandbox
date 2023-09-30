#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    auto context = create_graphics_context();

    const std::string modelsDirectory = MODELS_DIRECTORY;
    const std::string filePath = modelsDirectory + "/monu16.vox";
    auto test_scene = test_loader(filePath, chunk_manager, context);

    while (!should_exit(context)) {
        render_frame(context, test_scene);
    }
}
