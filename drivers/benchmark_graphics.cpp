#include <graphics/GraphicsContext.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    auto window = create_window();
    auto context = create_graphics_context(window);
    const size_t num_objects = 50;
    for (size_t i = 0; i < num_objects; ++i) {
        const float linear =
            static_cast<float>(i) / static_cast<float>(num_objects);
        const float radians = linear * 2.0F * 3.1415926;
        auto test_sphere_data =
            generate_basic_sphere_chunk(64, 64, 64, 4.0F + 60.0F * linear);
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

    while (!window->should_close()) {
        window->poll_events();
        camera->handle_keys(0.0001f);
        auto camera_info = camera->get_uniform_buffer();
        render_frame(context, scene, camera_info);
        camera->mark_rendered();
    }
}
