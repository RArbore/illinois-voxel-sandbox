#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include "voxels/AnvilLoader.h"

int main(int argc, char *argv[]) {
    std::shared_ptr<ChunkManager> chunk_manager = std::make_shared<ChunkManager>();
    auto window = create_window();
    auto context = create_graphics_context(window);

    const std::string modelsDirectory = MODELS_DIRECTORY;
    const std::string filePath = modelsDirectory + "/r.1.0.mca";
    AnvilLoader loader(filePath, context, chunk_manager);
    auto test_scene = loader.build_scene();

    glm::vec3 camera_pos = glm::vec3(512.0f, 0.0f, 48.0f);
    camera_pos *= 16;
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 0.1f, 0.25f);

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, test_scene, camera->get_position(), camera->get_front(), camera_info);
        camera->handle_keys(dt);
        camera->mark_rendered();
    }
}
