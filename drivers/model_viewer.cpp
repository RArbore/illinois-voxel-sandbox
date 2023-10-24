#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/Voxelize.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    const std::string modelsDirectory = MODELS_DIRECTORY;
    uint32_t chunk_width, chunk_height, chunk_depth;
    auto dragon = raw_voxelize_obj(modelsDirectory + "/dragon.obj", 0.005f, chunk_width, chunk_height, chunk_depth);

    VoxelChunkPtr test_dragon = chunk_manager.add_chunk(
							std::move(dragon), chunk_width, chunk_height, chunk_depth, VoxelChunk::Format::Raw,
							VoxelChunk::AttributeSet::Color);

    auto window = create_window();
    auto context = create_graphics_context(window);
    auto dragon_model = build_model(context, test_dragon);
    glm::mat3x4 dragon_transform = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
				    0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    auto dragon_object = build_object(context, dragon_model, dragon_transform);
    auto scene = build_scene(context, {dragon_object});

    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -250.0f);
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 0.1f, 0.25f);

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera->get_position(),
                                 camera->get_front(), camera_info);
        camera->handle_keys(dt);
        camera->mark_rendered();
    }
}
