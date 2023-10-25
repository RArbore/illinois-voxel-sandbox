#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/Voxelize.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    const std::string modelsDirectory = MODELS_DIRECTORY;
    uint32_t chunk_width, chunk_height, chunk_depth;
    auto tree = raw_voxelize_obj(modelsDirectory + "/hairball/hairball.obj", 0.02f, chunk_width, chunk_height, chunk_depth);
    auto svdag_tree = convert_raw_to_svdag(tree, chunk_width, chunk_height, chunk_depth, 4);
    std::cout << "SVDAG Size: " << svdag_tree.size() << "\n";

    VoxelChunkPtr test_tree = chunk_manager.add_chunk(
							std::move(svdag_tree), chunk_width, chunk_height, chunk_depth, VoxelChunk::Format::SVDAG,
							VoxelChunk::AttributeSet::Color);

    auto window = create_window();
    auto context = create_graphics_context(window);
    auto tree_model = build_model(context, test_tree);
    glm::mat3x4 tree_transform = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
				    0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    for (int x = 0; x < 1; ++x) {
	for (int z = 0; z < 1; ++z) {
	    tree_transform[0][3] = x * 1000;
	    tree_transform[2][3] = z * 1000;
	    auto tree_object = build_object(context, tree_model, tree_transform);
	    objects.emplace_back(std::move(tree_object));
	}
    }
    auto scene = build_scene(context, objects);

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
