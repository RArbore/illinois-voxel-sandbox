#include <graphics/GraphicsContext.h>
#include <graphics/Camera.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    auto test_proc_data1 = generate_basic_procedural_chunk(128, 128, 128);
    std::cout << "Raw size: " << test_proc_data1.size() << " bytes.\n";

    ChunkManager chunk_manager;
    auto window = create_window();
    auto context = create_graphics_context(window);
    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -250.0f);
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 0.1f, 0.25f);

    VoxelChunkPtr test_proc1 = chunk_manager.add_chunk(
        std::move(test_proc_data1), 128, 128, 128, VoxelChunk::Format::Raw,
        VoxelChunk::AttributeSet::Color);

    auto model1 = build_model(context, test_proc1);
    glm::mat3x4 transform1 = {1.0F, 0.0F,   0.0F, -64.0F, 0.0F, 1.0F,
                              0.0F, -64.0F, 0.0F, 0.0F, 1.0F, -64.0F};
    auto object1 = build_object(context, model1, transform1);
    auto scene = build_scene(context, {object1});

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera_info);
        camera->handle_keys(dt);
        camera->mark_rendered();
    }
}
