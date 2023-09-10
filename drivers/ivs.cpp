#include <graphics/GraphicsContext.h>
#include <voxels/Voxel.h>

int main(int argc, char *argv[]) {
    ChunkManager chunk_manager;
    auto test_cube_data1 = generate_basic_sphere_chunk(64, 64, 64, 32);
    auto test_cube_data2 = generate_basic_sphere_chunk(64, 64, 64, 20);
    VoxelChunkPtr test_cube1 = chunk_manager.add_chunk(std::move(test_cube_data1), 64, 64, 64, VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Color);
    VoxelChunkPtr test_cube2 = chunk_manager.add_chunk(std::move(test_cube_data2), 64, 64, 64, VoxelChunk::Format::Raw, VoxelChunk::AttributeSet::Color);
    
    auto context = create_graphics_context();

    auto model1 = build_model(context, test_cube1);
    auto model2 = build_model(context, test_cube2);
    auto model3 = build_model(context, test_cube2);
    glm::mat3x4 transform1 = {1.0F, 0.0F, 0.0F, 0.0F,
			      0.0F, 1.0F, 0.0F, 0.0F,
			      0.0F, 0.0F, 1.0F, 0.0F};
    glm::mat3x4 transform2 = {1.0F, 0.0F, 0.0F, 4.0F,
			      0.0F, 1.0F, 0.0F, 0.0F,
			      0.0F, 0.0F, 1.0F, 0.0F};
    glm::mat3x4 transform3 = {cosf(4.0F), -sinf(4.0F), 0.0F, 0.0F,
			      sinf(4.0F), cosf(4.0F), 0.0F, 1.0F,
			      0.0F, 0.0F, 1.0F, 0.0F};
    auto object1 = build_object(context, model1, transform1);
    auto object2 = build_object(context, model2, transform2);
    auto object3 = build_object(context, model2, transform3);
    auto scene = build_scene(context, {object1, object2, object3});

    while (!should_exit(context)) {
        render_frame(context, scene);
    }
}
