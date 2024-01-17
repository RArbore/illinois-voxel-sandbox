#include <fstream>

#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include <voxels/Voxelize.h>

int main(int argc, char *argv[]) {
    auto window = create_window();
    auto context = create_graphics_context(window);
    ChunkManager chunk_manager;

    const std::string models_directory = MODELS_DIRECTORY;
    std::string model_path = models_directory + "/sponza";

    std::ifstream stream(model_path, std::ios::in | std::ios::binary);
    const auto file_size = std::filesystem::file_size(model_path);
    std::vector<std::byte> svdag = std::vector<std::byte>(file_size);
    stream.read(reinterpret_cast<char *>(svdag.data()), file_size);

    uint32_t *svdag_ptr = reinterpret_cast<uint32_t *>(svdag.data());
    uint32_t chunk_width = svdag_ptr[0], chunk_height = svdag_ptr[1],
             chunk_depth = svdag_ptr[2];
    std::cout << "Sponza SVDAG Size: " << svdag.size() << "   " << chunk_width
              << " x " << chunk_height << " x " << chunk_depth << "\n";
    VoxelChunkPtr chunk = chunk_manager.add_chunk(
        std::move(svdag), chunk_width, chunk_height, chunk_depth,
        VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color);

    std::vector<std::shared_ptr<GraphicsObject>> objects;

    // Add Sponza
    auto sponza_model = build_model(context, chunk);
    const uint32_t min_dim =
        chunk_width < chunk_height
            ? chunk_width < chunk_depth ? chunk_width : chunk_depth
        : chunk_height < chunk_depth ? chunk_height
                                     : chunk_depth;
    const float size = 100.0 / static_cast<float>(min_dim);
    glm::mat3x4 sponza_transform = {size, 0.0F, 0.0F, 0.0F, 0.0F, size,
                                    0.0F, 0.0F, 0.0F, 0.0F, size, 0.0F};
    auto sponza_object = build_object(context, sponza_model, sponza_transform);
    objects.emplace_back(std::move(sponza_object));

    // Add emissive voxels
    auto emissive_block = generate_basic_filled_chunk(8, 8, 8);
    VoxelChunkPtr test_light = chunk_manager.add_chunk(
        std::move(emissive_block), 8, 8, 8, VoxelChunk::Format::Raw,
        VoxelChunk::AttributeSet::Emissive);
    auto light = build_model(context, test_light);

    glm::mat3x4 light_transform_1 = {1.0F, 0.0F,  0.0F, 40.0F, 0.0F, 1.0F,
                                     0.0F, 50.0F, 0.0F, 0.0F,  1.0F, 45.0F};
    auto light_object_1 = build_object(context, light, light_transform_1);
    objects.emplace_back(std::move(light_object_1));

    glm::mat3x4 light_transform_2 = {1.0F, 0.0F,  0.0F, 40.0F, 0.0F, 1.0F,
                                     0.0F, 50.0F, 0.0F, 0.0F,  1.0F, 95.0F};
    auto light_object_2 = build_object(context, light, light_transform_2);
    objects.emplace_back(std::move(light_object_2));

    glm::mat3x4 light_transform_3 = {1.0F, 0.0F,  0.0F, 125.0F, 0.0F, 1.0F,
                                     0.0F, 75.0F, 0.0F, 0.0F,   1.0F, 70.0F};
    auto light_object_3 = build_object(context, light, light_transform_3);
    objects.emplace_back(std::move(light_object_3));

    glm::mat3x4 light_transform_4 = {0.5F, 0.0F,  0.0F, 45.0F, 0.0F, 0.5F,
                                     0.0F, 75.0F, 0.0F, 0.0F,  0.5F, 45.0F};
    auto light_object_4 = build_object(context, light, light_transform_4);
    objects.emplace_back(std::move(light_object_4));

    glm::mat3x4 light_transform_5 = {0.5F, 0.0F,  0.0F, 45.0F, 0.0F, 0.5F,
                                     0.0F, 75.0F, 0.0F, 0.0F,  0.5F, 95.0F};
    auto light_object_5 = build_object(context, light, light_transform_5);
    objects.emplace_back(std::move(light_object_5));

    auto scene = build_scene(context, objects);

    glm::vec3 camera_pos = glm::vec3(72.5f, 75.0f, 75.0f);
    auto camera = create_camera(window, camera_pos, 0.0f, 90.0f, 0.1f, 0.25f);

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera->get_position(),
                                 camera->get_front(), camera_info);
        camera->mark_rendered();
        camera->handle_keys(dt);
    }
}
