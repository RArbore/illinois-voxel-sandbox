#include <graphics/Camera.h>
#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>

int main(int argc, char *argv[]) {
    
    // std::cout << "Raw size: " << test_proc_data1.size() << " bytes.\n";
    /*auto test_svo_data1 =
        convert_raw_to_svo(std::move(test_proc_data1), 128, 128, 128, 4);
	std::cout << "SVO size: " << test_svo_data1.size() << " bytes.\n";*/

    ChunkManager chunk_manager;
    auto window = create_window();
    auto context = create_graphics_context(window);
    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -250.0f);
    auto camera = create_camera(window, camera_pos, 0.0f, 0.0f, 0.1f, 0.25f);


    // auto test_proc_data1 = generate_basic_procedural_chunk(128, 128, 128);
    // VoxelChunkPtr test_proc1 = chunk_manager.add_chunk(
    //     std::move(test_proc_data1), 128, 128, 128, VoxelChunk::Format::Raw,
    //     VoxelChunk::AttributeSet::Color);
    // auto model1 = build_model(context, test_proc1);
    // glm::mat3x4 transform1 = {1.0F, 0.0F,   0.0F, -64.0F, 0.0F, 1.0F,
    //                           0.0F, -64.0F, 0.0F, 0.0F,   1.0F, -64.0F};


    int size = 128;
    auto rock_data = generate_procedural_rock(size, size, size, 1500);
    auto proc_svo =
        convert_raw_to_svo(std::move(rock_data), size, size, size, 4);
    VoxelChunkPtr rock = chunk_manager.add_chunk(
        std::move(proc_svo), size, size, size, VoxelChunk::Format::SVO,
        VoxelChunk::AttributeSet::Color);
    auto rock_model = build_model(context, rock);

    
    int numobject = 20;
    auto objects = std::vector<std::shared_ptr<GraphicsObject>>{};
    for (uint16_t i = 0; i < numobject; i++) {
        //give each model a random position and rotation
        float x = (rand() % 100);
        float y = (rand() % 100);
        float z = (rand() % 100);
        float radius = 50;
        float r = radius * sqrt((rand() % 1000) / 1000.0F);
        float theta = ((rand() % 1000) / 1000.0F) * 2 * 3.14159265358979323846;
        glm::mat3x4 translation = {0.0F, 0.0F, 0.0F, r * cos(theta), 
                                   0.0F, 0.0F, 0.0F, 0, 
                                   0.0F, 0.0F, 0.0F, r * sin(theta)};

        glm::mat3x3 rotationx = {1.0F,        0.0F,         0.0F,
                                 0.0F, cos(x * 10), -sin(x * 10),
                                 0.0F, sin(x * 10),  cos(x * 10)};

        glm::mat3x3 rotationy = {cos(y * 10), 0.0F, sin(y * 10),
                                 0.0F,        1.0F,        0.0F,
                                -sin(y * 10), 0.0F, cos(y * 10)};

        glm::mat3x3 rotationz = {cos(z * 10), -sin(z * 10), 0.0F,
                                 sin(z * 10),  cos(z * 10), 0.0F,
                                 0.0F,         0.0F,        1.0F};
        float scaleFactor = 1;
        glm::mat3x3 scale = {scaleFactor, 0.0F, 0.0F,
                             0.0F, scaleFactor, 0.0F,
                             0.0F, 0.0F, scaleFactor};
        glm::mat3x3 transform = scale * rotationx * rotationy * rotationz;
        glm::mat3x4 castTransform = glm::mat3x4(transform);
        castTransform = castTransform + translation;
        auto object1 = build_object(context, rock_model, castTransform);
        objects.push_back(object1);
    }
    
      

    int island_size = 128;
    auto island_data = generate_island_chunk(island_size, 50, 100);
    auto proc_island_svo =
        convert_raw_to_svo(std::move(island_data), island_size, island_size, island_size, 4);
    VoxelChunkPtr island = chunk_manager.add_chunk(
        std::move(proc_island_svo), island_size, island_size, island_size, VoxelChunk::Format::SVO,
        VoxelChunk::AttributeSet::Color);
    auto island_model = build_model(context, island);
    glm::mat3x4 transform1 = { 1.0F, 0.0F, 0.0F, 0.0F, 
                                0.0F, 1.0F, 0.0F, 0.0F, 
                                0.0F, 0.0F, 1.0F, 0.0F};
    auto island_object = build_object(context, island_model, transform1);

    objects.push_back(island_object);

    auto scene = build_scene(context, {objects}); 

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera->get_position(),
                                 camera->get_front(), camera_info);
        camera->handle_keys(dt);
        camera->mark_rendered();
    }
}
