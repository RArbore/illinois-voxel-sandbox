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
    auto test_island_data = generate_island_chunk(size, 50, 100);
    VoxelChunkPtr test_island = chunk_manager.add_chunk(
        std::move(test_island_data), size, size, size, VoxelChunk::Format::Raw,
        VoxelChunk::AttributeSet::Color);
    auto model1 = build_model(context, test_island);
    glm::mat3x4 transform1 = {1.0F, 0.0F,   0.0F, -4.0F, 0.0F, 1.0F,
                              0.0F, -4.0F, 0.0F, 0.0F,   1.0F, -4.0F};

    
    auto object1 = build_object(context, model1, transform1);
    auto scene = build_scene(context, {object1});




    double elapsed = 0.0;
    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera->get_position(),
                                 camera->get_front(), camera_info);
        camera->handle_keys(dt);
        camera->mark_rendered();

        elapsed += dt / 1000.0;
        if (elapsed >= 1.0) {
            chunk_manager.debug_print();
            elapsed = 0.0;
        }
    }
}
