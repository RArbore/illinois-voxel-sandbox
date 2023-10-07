#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    auto window = create_window();
    auto context = create_graphics_context(window);

    const std::string modelsDirectory = MODELS_DIRECTORY;
    const std::string filePath = modelsDirectory + "/carsmodified.vox";
    auto test_scene = load_vox_scene(filePath, chunk_manager, context);

    while (!window->should_close()) {
        window->poll_events();
        camera->handle_keys(0.0001f);
        auto camera_info = camera->get_uniform_buffer();
        render_frame(context, scene, camera_info);
        camera->mark_rendered();
    }
}
