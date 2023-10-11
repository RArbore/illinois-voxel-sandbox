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

    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -250.0f);
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 100.0f, 0.05f);

    while (!window->should_close()) {
        window->poll_events();
        camera->handle_keys(0.0001f);
        auto camera_info = camera->get_uniform_buffer();
        render_frame(context, test_scene, camera_info);
        camera->mark_rendered();
    }
}
