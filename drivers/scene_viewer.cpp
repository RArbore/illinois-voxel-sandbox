#include <iostream>
#include <fstream>

#include <nlohmann_json.hpp>
using json = nlohmann::json;

#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>

std::shared_ptr<Window> window = nullptr;
std::shared_ptr<GraphicsContext> context = nullptr;
std::shared_ptr<Camera> camera = nullptr;
std::shared_ptr<GraphicsScene> scene = nullptr;

void parse_scene(const std::string& scene_path) {
    std::ifstream scene_description_file(scene_path);
    json scene_layout_description = json::parse(scene_description_file);
    
    json &camera_description = scene_layout_description["camera"];
    ASSERT(!camera_description.is_null(), "Camera description must be provided!");
 
    json& position = camera_description["position"];
    json &pitch = camera_description["pitch"];
    json &yaw = camera_description["yaw"];
    json &speed = camera_description["speed"];
    json &sensitivity = camera_description["sensitivity"];

    ASSERT(!position.is_null(), "Camera position must be provided!");
    ASSERT(!pitch.is_null(), "Camera pitch must be provided!");
    ASSERT(!yaw.is_null(), "Camera yaw must be provided!");
    ASSERT(!speed.is_null(), "Camera speed must be provided!");
    ASSERT(!sensitivity.is_null(), "Camera sensitivity must be provided!");
    
    glm::vec3 camera_pos = glm::vec3(position[0], position[1], position[2]);
    camera = create_camera(window, camera_pos, 
                                   pitch.get<float>(), yaw.get<float>(), 
                                   speed.get<float>(), sensitivity.get<float>());

    json &scene_description = scene_layout_description["scene"];
    ASSERT(!scene_description.is_null(), "Scene description must be provided!");
}

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a scene to render.");
    std::string scene_path(argv[1]);

    std::ifstream scene_description_file(scene_path);
    json scene_description = json::parse(scene_description_file);

    window = create_window();
    context = create_graphics_context(window);
    parse_scene(argv[1]);

    // Temporarily building the scene manually
    ChunkManager chunk_manager;
    auto test_proc_data1 = generate_basic_procedural_chunk(128, 128, 128);
    auto test_svdag_data1 =
        convert_raw_to_svdag(std::move(test_proc_data1), 128, 128, 128, 4);
    VoxelChunkPtr test_proc1 = chunk_manager.add_chunk(
        std::move(test_svdag_data1), 128, 128, 128, VoxelChunk::Format::SVDAG,
        VoxelChunk::AttributeSet::Color);
    auto model1 = build_model(context, test_proc1);
    glm::mat3x4 transform1 = {1.0F, 0.0F,   0.0F, -64.0F, 0.0F, 1.0F,
                              0.0F, -64.0F, 0.0F, 0.0F,   1.0F, -64.0F};
    auto object1 = build_object(context, model1, transform1);

    auto emissive_block =
        append_metadata_to_raw(generate_basic_filled_chunk(8, 8, 8), 8, 8, 8);
    VoxelChunkPtr test_light = chunk_manager.add_chunk(
        std::move(emissive_block), 8, 8, 8, VoxelChunk::Format::Raw,
        VoxelChunk::AttributeSet::Emissive);
    auto light = build_model(context, test_light);
    glm::mat3x4 transform2 = {2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.0F,
                              0.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0.0F};
    auto object2 = build_object(context, light, transform2);

    auto scene = build_scene(context, {object1, object2});

    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, 
                                 camera->get_position(),
                                 camera->get_front(), 
                                 camera_info);
        camera->mark_rendered();
        camera->handle_keys(dt);
    }
}