#include <fstream>

#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include <voxels/Voxelize.h>
#include <external/FastNoiseLite.h>

int main(int argc, char *argv[]) {
    auto window = create_window();
    auto context = create_graphics_context(window);
    ChunkManager chunk_manager;
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    const std::string models_directory = MODELS_DIRECTORY;

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(456);
    
    // std::vector<std::string> model_paths = {"/white_oak_1", "/wheat01", "/stones002", "/grass", "/basalt"};
    // for (uint32_t i = 0; i < model_paths.size(); ++i) {
    //     std::string path = models_directory + model_paths[i];
    //     std::ifstream stream(path, std::ios::in | std::ios::binary);
    //     const auto file_size = std::filesystem::file_size(path);
    //     std::vector<std::byte> svdag = std::vector<std::byte>(file_size);
    //     stream.read(reinterpret_cast<char *>(svdag.data()), file_size);
    //     std::cout << model_paths[i] << " SVDAG Size: " << svdag.size() << "\n";

    //     uint32_t *svdag_ptr = reinterpret_cast<uint32_t *>(svdag.data());
    //     uint32_t chunk_width = svdag_ptr[0], 
    //              chunk_height = svdag_ptr[1],
    //              chunk_depth = svdag_ptr[2];
    //     VoxelChunkPtr chunk = chunk_manager.add_chunk(
    //         std::move(svdag), chunk_width, chunk_height, chunk_depth,
    //         VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color);


    //     auto model = build_model(context, chunk);
    //     const uint32_t min_dim =
    //         chunk_width < chunk_height
    //             ? chunk_width < chunk_depth ? chunk_width : chunk_depth
    //         : chunk_height < chunk_depth ? chunk_height
    //                                      : chunk_depth;
    //     const float size = 100.0 / static_cast<float>(min_dim);
    //     glm::mat3x4 transform = {size, 0.0F, 0.0F, 0.0F, 0.0F, size,
    //                                     0.0F, 0.0F, 0.0F, 0.0F, size, 0.0F};
    //     auto object = build_object(context, model, transform);
    //     objects.emplace_back(std::move(object));
    // }





    std::string tree_model_path = models_directory + "/white_oak_1";
    std::ifstream tree_stream(tree_model_path, std::ios::in | std::ios::binary);
    const auto tree_file_size = std::filesystem::file_size(tree_model_path);
    std::vector<std::byte> tree_svdag = std::vector<std::byte>(tree_file_size);
    tree_stream.read(reinterpret_cast<char *>(tree_svdag.data()), tree_file_size);
    std::cout << "Tree SVDAG Size: " << tree_svdag.size() << "\n";

    uint32_t *tree_svdag_ptr = reinterpret_cast<uint32_t *>(tree_svdag.data());
    uint32_t chunk_width = tree_svdag_ptr[0], 
             chunk_height = tree_svdag_ptr[1],
             chunk_depth = tree_svdag_ptr[2];
    VoxelChunkPtr tree_chunk = chunk_manager.add_chunk(
        std::move(tree_svdag), chunk_width, chunk_height, chunk_depth,
        VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color);

    auto tree_model = build_model(context, tree_chunk);
    const uint32_t min_dim =
        chunk_width < chunk_height
            ? chunk_width < chunk_depth ? chunk_width : chunk_depth
        : chunk_height < chunk_depth ? chunk_height
                                     : chunk_depth;
    const float size = 100.0 / static_cast<float>(min_dim);
    for (uint32_t i = 0; i < 10; i++) {
        float x = (rand() % 100);
        float y = (rand() % 100);
        float z = (rand() % 100);
        float radius = 200;
        float r = radius * sqrt((rand() % 1000) / 1000.0F);
        float theta = ((rand() % 1000) / 1000.0F) * 2 * 3.14159265358979323846;
        float offset = noise.GetNoise((r * cos(theta) + 150) / 10, (r * sin(theta) + 150) / 10) * 10;
        glm::mat3x4 randomtranslation = {0.0F, 0.0F, 0.0F, r * cos(theta), 
                                   0.0F, 0.0F, 0.0F, 0, 
                                   0.0F, 0.0F, 0.0F, r * sin(theta)};
        glm::mat3x4 customtranslation = {0.0F, 0.0F, 0.0F, 150.0F, 
                                         0.0F, 0.0F, 0.0F, -10.0F + offset, 
                                         0.0F, 0.0F, 0.0F, 150.0F};

        glm::mat3x3 rotationx = {1.0F,        0.0F,         0.0F,
                                 0.0F, cos(x * 10), -sin(x * 10),
                                 0.0F, sin(x * 10),  cos(x * 10)};

        glm::mat3x3 rotationy = {cos(y * 10), 0.0F, sin(y * 10),
                                 0.0F,        1.0F,        0.0F,
                                -sin(y * 10), 0.0F, cos(y * 10)};

        glm::mat3x3 rotationz = {cos(z * 10), -sin(z * 10), 0.0F,
                                 sin(z * 10),  cos(z * 10), 0.0F,
                                 0.0F,         0.0F,        1.0F};
        float scaleFactor = size;
        glm::mat3x3 scale = {scaleFactor, 0.0F, 0.0F,
                             0.0F, scaleFactor, 0.0F,
                             0.0F, 0.0F, scaleFactor};
        glm::mat3x3 transform = scale * rotationy;
        glm::mat3x4 cast_transform = glm::mat3x4(transform);
        cast_transform = cast_transform + randomtranslation + customtranslation;
        auto tree_object = build_object(context, tree_model, cast_transform);
        objects.emplace_back(std::move(tree_object));
    }

    //add the stones
    std::string stone_model_path = models_directory + "/stones002";
    std::ifstream stone_stream(stone_model_path, std::ios::in | std::ios::binary);
    const auto stone_file_size = std::filesystem::file_size(stone_model_path);
    std::vector<std::byte> stone_svdag = std::vector<std::byte>(stone_file_size);
    stone_stream.read(reinterpret_cast<char *>(stone_svdag.data()), stone_file_size);
    std::cout << "Stone SVDAG Size: " << stone_svdag.size() << "\n";

    uint32_t *stone_svdag_ptr = reinterpret_cast<uint32_t *>(stone_svdag.data());
    chunk_width = stone_svdag_ptr[0], 
             chunk_height = stone_svdag_ptr[1],
             chunk_depth = stone_svdag_ptr[2];
    VoxelChunkPtr stone_chunk = chunk_manager.add_chunk(
        std::move(stone_svdag), chunk_width, chunk_height, chunk_depth,
        VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color);
    
    auto stone_model = build_model(context, stone_chunk);
    const uint32_t stone_min_dim =
        chunk_width < chunk_height
            ? chunk_width < chunk_depth ? chunk_width : chunk_depth
        : chunk_height < chunk_depth ? chunk_height
                                     : chunk_depth;
    const float stone_size = (100.0 / static_cast<float>(stone_min_dim)) * 0.04;
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < 10; j++){
            float offset = noise.GetNoise((static_cast<float>(i) * 60 - 200) / 10, (static_cast<float>(j) * 60  - 200)/ 10) * 10;
            glm::mat3x4 stone_transform = {0.0F, 0.0F, 0.0F, i * 60.0F, 
                                        0.0F, 0.0F, 0.0F, 90.0F + offset, 
                                        0.0F, 0.0F, 0.0F, j * 60.0F};
            glm::mat3x4 custom_stone_transform = {0.0F, 0.0F, 0.0F, -200.0F, 
                                        0.0F, 0.0F, 0.0F, 0.0F, 
                                        0.0F, 0.0F, 0.0F, -200.0F};
            float y = (rand() % 100) - 50;
            glm::mat3x3 rotationy = {cos(y / 200), 0.0F, sin(y / 200),
                                    0.0F,        1.0F,        0.0F,
                                    -sin(y / 200), 0.0F, cos(y / 200)};
            float scaleFactor = size;
            glm::mat3x3 scale = {stone_size, 0.0F, 0.0F,
                                0.0F, stone_size, 0.0F,
                                0.0F, 0.0F, stone_size};
            glm::mat3x3 transform = scale * rotationy;
            glm::mat3x4 cast_transform = glm::mat3x4(transform);
            cast_transform = cast_transform + stone_transform + custom_stone_transform;
            auto stone_object = build_object(context, stone_model, cast_transform);
            objects.emplace_back(std::move(stone_object));
        }
    }

    //add the wheat
    std::string wheat_model_path = models_directory + "/wheat01";
    std::ifstream wheat_stream(wheat_model_path, std::ios::in | std::ios::binary);
    const auto wheat_file_size = std::filesystem::file_size(wheat_model_path);
    std::vector<std::byte> wheat_svdag = std::vector<std::byte>(wheat_file_size);
    wheat_stream.read(reinterpret_cast<char *>(wheat_svdag.data()), wheat_file_size);
    std::cout << "Wheat SVDAG Size: " << wheat_svdag.size() << "\n";

    uint32_t *wheat_svdag_ptr = reinterpret_cast<uint32_t *>(wheat_svdag.data());
    chunk_width = wheat_svdag_ptr[0], 
             chunk_height = wheat_svdag_ptr[1],
             chunk_depth = wheat_svdag_ptr[2];
    VoxelChunkPtr wheat_chunk = chunk_manager.add_chunk(
        std::move(wheat_svdag), chunk_width, chunk_height, chunk_depth,
        VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color);

    auto wheat_model = build_model(context, wheat_chunk);
    const uint32_t wheat_min_dim =
        chunk_width < chunk_height
            ? chunk_width < chunk_depth ? chunk_width : chunk_depth
        : chunk_height < chunk_depth ? chunk_height
                                     : chunk_depth;
    const float wheat_size = (100.0 / static_cast<float>(wheat_min_dim)) * 0.04;

    for (uint32_t i = 0; i < 5000; i++) {
        float x = (rand() % 100);
        float y = (rand() % 100);
        float z = (rand() % 100);
        float radius = 300;
        float r = radius * sqrt((rand() % 1000) / 1000.0F);
        float theta = ((rand() % 1000) / 1000.0F) * 2 * 3.14159265358979323846;
        float offset = noise.GetNoise((r * cos(theta) + 150) / 1, (r * sin(theta) + 150) / 1) * 10;
        glm::mat3x4 randomtranslation = {0.0F, 0.0F, 0.0F, r * cos(theta), 
                                   0.0F, 0.0F, 0.0F, 0, 
                                   0.0F, 0.0F, 0.0F, r * sin(theta)};
        glm::mat3x4 customtranslation = {0.0F, 0.0F, 0.0F, 150.0F, 
                                         0.0F, 0.0F, 0.0F, 70.0F + offset, 
                                         0.0F, 0.0F, 0.0F, 150.0F};

        glm::mat3x3 rotationy = {cos(y * 10), 0.0F, sin(y * 10),
                                 0.0F,        1.0F,        0.0F,
                                -sin(y * 10), 0.0F, cos(y * 10)};
        float scaleFactor = wheat_size;
        glm::mat3x3 scale = {scaleFactor, 0.0F, 0.0F,
                             0.0F, scaleFactor, 0.0F,
                             0.0F, 0.0F, scaleFactor};
        glm::mat3x3 transform = scale * rotationy;
        glm::mat3x4 cast_transform = glm::mat3x4(transform);
        cast_transform = cast_transform + randomtranslation + customtranslation;
        auto wheat_object = build_object(context, wheat_model, cast_transform);
        objects.emplace_back(std::move(wheat_object));
    }
    

    // Add emissive voxels
    auto emissive_block = generate_basic_filled_chunk(8, 8, 8);
    VoxelChunkPtr test_light = chunk_manager.add_chunk(
        std::move(emissive_block), 8, 8, 8, VoxelChunk::Format::Raw,
        VoxelChunk::AttributeSet::Emissive);
    auto light = build_model(context, test_light);

    glm::mat3x4 light_transform_1 = {1.0F, 0.0F, 0.0F, 40.0F, 0.0F, 1.0F,
                              0.0F, 50.0F, 0.0F, 0.0F, 1.0F, 45.0F};
    auto light_object_1 = build_object(context, light, light_transform_1);
    objects.emplace_back(std::move(light_object_1));
   
    glm::mat3x4 light_transform_2 = {1.0F, 0.0F,  0.0F, 40.0F, 0.0F, 1.0F,
                                     0.0F, 50.0F, 0.0F, 0.0F,  1.0F, 95.0F};
    auto light_object_2 = build_object(context, light, light_transform_2);
    objects.emplace_back(std::move(light_object_2));

    glm::mat3x4 light_transform_3 = {1.0F, 0.0F,  0.0F, 125.0F, 0.0F, 1.0F,
                                     0.0F, 75.0F, 0.0F, 0.0F,  1.0F, 70.0F};
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

    glm::vec3 camera_pos = glm::vec3(72.5f, 55.0f, 75.0f);
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