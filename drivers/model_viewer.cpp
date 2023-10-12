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
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 0.1f, 0.25f);

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, test_scene, camera->get_position(), camera->get_front(), camera_info);
        camera->handle_keys(dt);
        camera->mark_rendered();
    }
}
