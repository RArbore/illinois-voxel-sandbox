#include <graphics/GraphicsContext.h>
#include <voxels/Voxel.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    auto context = create_graphics_context();
    for (size_t i = 0; i < 25; ++i) {
	const float linear = static_cast<float>(i) / 25.0F;
	const float radians = linear * 2.0F * 3.1415926;
	auto test_sphere_data = generate_basic_sphere_chunk(64, 64, 64, 4.0F + 60.0F * linear);
	VoxelChunkPtr test_sphere = chunk_manager.add_chunk(std::move(test_sphere_data), 64, 64, 64, VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Color);
	
	auto model = build_model(context, test_sphere);
	glm::mat3x4 transform = {1.0F, 0.0F, 0.0F, 5.0F * cosf(radians),
				 0.0F, 1.0F, 0.0F, 0.0F,
				 0.0F, 0.0F, 1.0F, 5.0F * sinf(radians)};
	auto object = build_object(context, model, transform);
	objects.emplace_back(std::move(object));
    }
    auto scene = build_scene(context, std::move(objects));

    while (!should_exit(context)) {
        render_frame(context, scene);
    }
}
