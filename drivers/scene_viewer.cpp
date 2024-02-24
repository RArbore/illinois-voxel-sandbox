#include <iostream>
#include <filesystem>
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
ChunkManager chunk_manager;

void parse_scene(const std::string& scene_path) {
    std::ifstream scene_description_file(scene_path);
    json scene_layout_description = json::parse(scene_description_file);
    
    json &camera_description = scene_layout_description["camera"];
    ASSERT(!camera_description.is_null(), "Camera description must be provided!");
 
    json& json_position = camera_description["position"];
    json& json_pitch = camera_description["pitch"];
    json& json_yaw = camera_description["yaw"];
    json& json_speed = camera_description["speed"];
    json& json_sensitivity = camera_description["sensitivity"];

    ASSERT(!json_position.is_null(), "Camera position must be provided!");
    ASSERT(!json_pitch.is_null(), "Camera pitch must be provided!");
    ASSERT(!json_yaw.is_null(), "Camera yaw must be provided!");
    ASSERT(!json_speed.is_null(), "Camera speed must be provided!");
    ASSERT(!json_sensitivity.is_null(), "Camera sensitivity must be provided!");
    
    glm::vec3 camera_pos = glm::vec3(json_position[0], json_position[1], json_position[2]);
    camera = create_camera(window, camera_pos, 
                                   json_pitch.get<float>(), json_yaw.get<float>(), 
                                   json_speed.get<float>(), json_sensitivity.get<float>());

    json &scene_description = scene_layout_description["scene"];
    ASSERT(!scene_description.is_null(), "Scene description must be provided!");

    std::vector<std::shared_ptr<GraphicsObject>> objects;
    for (json &scene_model : scene_description) {
        json& json_type = scene_model["type"];
        json& json_dimensions = scene_model["dimensions"];
        json& json_attributes = scene_model["attributes"];
        json& json_format = scene_model["format"];
        json& json_transform = scene_model["transform"];

        ASSERT(!json_type.is_null(), "Model type must be provided!");
        ASSERT(!json_dimensions.is_null(), "Model dimensions must be provided!");
        ASSERT(!json_attributes.is_null(), "Model attributes must be provided!");
        ASSERT(!json_format.is_null(), "Model format must be provided!");
        ASSERT(!json_transform.is_null(), "Model transform must be provided!");

        std::string type = json_type.get<std::string>();
        std::string attributes = json_attributes.get<std::string>();
        std::string format = json_format.get<std::string>();

        int width = json_dimensions[0];
        int height = json_dimensions[1];
        int depth = json_dimensions[2];

        std::vector<std::byte> raw_data;
        bool procedural = true;
        if (type == "basic_procedural") {
            raw_data = generate_basic_procedural_chunk(width, height, depth);
        } else if (type == "basic_filled") {
            raw_data = generate_basic_filled_chunk(width, height, depth);
        } else {
            // If it's not a basic procedural type, assume that it's a voxelized model
            procedural = false;

            // Assume that the model path is referring to a relative path from the JSON itself
            std::filesystem::path parent = std::filesystem::path(scene_path).parent_path();
            std::filesystem::path model_path = parent / type;

            std::ifstream stream(model_path, std::ios::in | std::ios::binary);
            const auto file_size = std::filesystem::file_size(model_path);
            raw_data = std::vector<std::byte>(file_size);
            stream.read(reinterpret_cast<char *>(raw_data.data()), file_size);
        }

        VoxelChunk::Format voxel_format;
        if (format == "raw") {
            voxel_format = VoxelChunk::Format::Raw;
            if (procedural) {
                raw_data = append_metadata_to_raw(raw_data, width, height, depth);
            }
        } else if (format == "svo") {
            voxel_format = VoxelChunk::Format::SVO;
            if (procedural) {
                raw_data = convert_raw_to_svo(std::move(raw_data), width, height, depth, 4);
            }
        } else if (format == "svdag") {
            voxel_format = VoxelChunk::Format::SVDAG;
            if (procedural) {
                raw_data = convert_raw_to_svdag(std::move(raw_data), width, height, depth, 4);
            }
        } else if (format == "df") {
            voxel_format = VoxelChunk::Format::DF; 
            if (procedural) {
                raw_data = convert_raw_color_to_df(raw_data, width, height, depth, 4);
                raw_data = append_metadata_to_raw(raw_data, width, height, depth);
            }
        } else {
            ASSERT(false, "Unrecognized voxel format provided: " + format);
        }

        VoxelChunk::AttributeSet voxel_attributes;
        if (attributes == "color") {
            voxel_attributes = VoxelChunk::AttributeSet::Color;
        } else if (attributes == "color_normal") {
            voxel_attributes = VoxelChunk::AttributeSet::ColorNormal;
        } else if (attributes == "emissive") {
            voxel_attributes = VoxelChunk::AttributeSet::Emissive;
        } else {
            ASSERT(false, "Unrecognized voxel attributes provided: " + attributes);
        }

        glm::mat3x4 object_transform = glm::mat3x4{
            json_transform[0], json_transform[1], json_transform[2], json_transform[3],
            json_transform[4], json_transform[5], json_transform[6], json_transform[7], 
            json_transform[8], json_transform[9], json_transform[10], json_transform[11]
        };

        VoxelChunkPtr chunk = chunk_manager.add_chunk(
            std::move(raw_data), 
            width, height, depth, 
            voxel_format, voxel_attributes);
        auto model = build_model(context, chunk);
        auto object = build_object(context, model, object_transform);
        objects.push_back(object);
    }

    scene = build_scene(context, objects);
}

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a scene to render.");
    ASSERT(std::filesystem::exists(argv[1]), "JSON file must exist.");
    std::string scene_path(argv[1]);

    std::ifstream scene_description_file(scene_path);
    json scene_description = json::parse(scene_description_file);

    window = create_window();
    context = create_graphics_context(window);
    parse_scene(argv[1]);

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