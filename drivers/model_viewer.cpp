#include <fstream>

#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/Voxelize.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a SVDAG model to render.");
    std::string model_path(argv[1]);

    ChunkManager chunk_manager;
    std::ifstream stream(model_path,
			 std::ios::in | std::ios::binary);
    const auto size = std::filesystem::file_size(model_path);
    std::vector<std::byte> svdag = std::vector<std::byte>(size);
    stream.read(reinterpret_cast<char*>(svdag.data()), size);
    std::cout << "SVDAG Size: " << svdag.size() << "\n";

    uint32_t chunk_width, chunk_height, chunk_depth;
    VoxelChunkPtr chunk = chunk_manager.add_chunk(
						  std::move(svdag), chunk_width, chunk_height, chunk_depth, VoxelChunk::Format::SVDAG,
						  VoxelChunk::AttributeSet::Color);
    
    auto window = create_window();
    auto context = create_graphics_context(window);
    auto model = build_model(context, chunk);
    glm::mat3x4 transform = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
			     0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    auto object = build_object(context, model, transform);
    objects.emplace_back(std::move(object));
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
