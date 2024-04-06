#include <cstring>
#include <fstream>

#include <compiler/Compiler.h>
#include <graphics/GraphicsContext.h>
#include <voxels/Conversion.h>
#include <voxels/Voxel.h>
#include <voxels/VoxelChunkGeneration.h>
#include <voxels/Voxelize.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a model to render.");
    ASSERT(argv[2], "Must provide a format of the model.");
    std::string model_path(argv[1]);

    ChunkManager chunk_manager;
    std::ifstream stream(model_path, std::ios::in | std::ios::binary);
    const auto file_size = std::filesystem::file_size(model_path);
    std::vector<std::byte> model_bytes = std::vector<std::byte>(file_size);
    stream.read(reinterpret_cast<char *>(model_bytes.data()), file_size);
    std::cout << "Model size: " << model_bytes.size() << " bytes\n";

    uint32_t *model_ptr = reinterpret_cast<uint32_t *>(model_bytes.data());
    VoxelChunkPtr chunk;
    uint32_t chunk_width, chunk_height, chunk_depth;
    if (!strcmp(argv[2], "svdag")) {
        chunk_width = model_ptr[0];
        chunk_height = model_ptr[1];
        chunk_depth = model_ptr[2];
        chunk = chunk_manager.add_chunk(
            std::move(model_bytes), chunk_width, chunk_height, chunk_depth,
            VoxelChunk::Format::SVDAG, VoxelChunk::AttributeSet::Color);
    } else if (!strcmp(argv[2], "raw")) {
        chunk_width = model_ptr[0];
        chunk_height = model_ptr[1];
        chunk_depth = model_ptr[2];
        chunk = chunk_manager.add_chunk(
            std::move(model_bytes), chunk_width, chunk_height, chunk_depth,
            VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Color);
    } else if (!strcmp(argv[2], "df")) {
        chunk_width = model_ptr[0];
        chunk_height = model_ptr[1];
        chunk_depth = model_ptr[2];
        chunk = chunk_manager.add_chunk(
            std::move(model_bytes), chunk_width, chunk_height, chunk_depth,
            VoxelChunk::Format::DF, VoxelChunk::AttributeSet::Color);
    } else {
        std::cout << "Interpreting " << argv[2] << " as a custom format.\n";
	auto format = parse_format(argv[2]);
        auto bounds = calculate_bounds(format);
        chunk_width = std::get<0>(bounds);
        chunk_height = std::get<1>(bounds);
        chunk_depth = std::get<2>(bounds);
        chunk = chunk_manager.add_chunk(std::move(model_bytes), chunk_width,
                                        chunk_height, chunk_depth, format_identifier(format));
    }

    auto window = create_window();
    auto context = create_graphics_context(window);
    auto model = build_model(context, chunk);
    const uint32_t min_dim =
        chunk_width < chunk_height
            ? chunk_width < chunk_depth ? chunk_width : chunk_depth
        : chunk_height < chunk_depth ? chunk_height
                                     : chunk_depth;
    const float size = 100.0 / static_cast<float>(min_dim);
    glm::mat3x4 transform = {size, 0.0F, 0.0F, 0.0F, 0.0F, size,
                             0.0F, 0.0F, 0.0F, 0.0F, size, 0.0F};
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    auto object = build_object(context, model, transform);
    objects.emplace_back(std::move(object));
    auto scene = build_scene(context, objects);

    float camera_x = 50.0f;
    float camera_y = 50.0f;
    float camera_z = 100.0f;
    float pitch = 0.0f;
    float yaw = 180.0f;

    if (argv[3]) {
	ASSERT(argv[4], "If providing camera information, must provide the camera Y.");
	ASSERT(argv[5], "If providing camera information, must provide the camera Z.");
	ASSERT(argv[6], "If providing camera information, must provide the camera pitch.");
	ASSERT(argv[7], "If providing camera information, must provide the camera yaw.");
	camera_x = atof(argv[3]);
	camera_y = atof(argv[4]);
	camera_z = atof(argv[5]);
	pitch = atof(argv[6]);
	yaw = atof(argv[7]);
    }

    glm::vec3 camera_pos = glm::vec3(camera_x, camera_y, camera_z);
    auto camera = create_camera(window, camera_pos, pitch, yaw, 0.1f, 0.25f);

    int num_frames = 0;
    double accum_time = 0.0;
    while (!window->should_close()) {
        window->poll_events();
        auto camera_info = camera->get_uniform_buffer();
        double dt = render_frame(context, scene, camera->get_position(),
                                 camera->get_front(), camera_info);
        camera->mark_rendered();
        camera->handle_keys(dt);
	accum_time += dt;
	if (accum_time > 6000.0 && argv[3]) {
	    std::cout << "INFO: Final FPS measurement: " << (static_cast<double>(num_frames) / 5.0) << "\n";
	    return 0;
	} else if (accum_time > 1000.0) {
	    ++num_frames;
	}
	if (accum_time > 1000.0 && num_frames % 500 == 0 && !argv[3]) {
	    std::cout << "INFO: Camera Position: " << camera->origin_.x << " " << camera->origin_.y << " " << camera->origin_.z << " " << camera->pitch_ << " " << camera->yaw_ << "\n";
	}
    }
}
