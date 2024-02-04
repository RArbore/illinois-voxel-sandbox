#include <graphics/GraphicsContext.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include <voxels/Conversion.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    auto window = create_window();
    auto context = create_graphics_context(window);
    const size_t num_objects = 25;
    for (size_t i = 0; i < num_objects; ++i) {
        const float linear =
            static_cast<float>(i) / static_cast<float>(num_objects);
        const float radians = linear * 2.0F * 3.1415926;
        auto test_sphere_data =
            append_metadata_to_raw(generate_basic_sphere_chunk(64, 64, 64, 4.0F + 60.0F * linear), 64, 64, 64);
        VoxelChunkPtr test_sphere = chunk_manager.add_chunk(
            std::move(test_sphere_data), 64, 64, 64, VoxelChunk::Format::Raw,
            VoxelChunk::AttributeSet::Color);

        auto model = build_model(context, test_sphere);
        glm::mat3x4 transform = {
            1.0F, 0.0F, 0.0F, 500.0F * cosf(radians),
            0.0F, 1.0F, 0.0F, 100.0F * cosf(radians * 4.0F),
            0.0F, 0.0F, 1.0F, 500.0F * sinf(radians)};
        auto object = build_object(context, model, transform);
        objects.emplace_back(std::move(object));
    }
    auto scene = build_scene(context, std::move(objects));

    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -5.0f);
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 0.1f, 0.25f);

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera->get_position(),
                                 camera->get_front(), camera_info);
        camera->mark_rendered();
        camera->handle_keys(dt);
    }
}
