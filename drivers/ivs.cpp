#include <graphics/GraphicsContext.h>
#include <voxels/VoxelManager.h>

int main(int argc, char *argv[]) {
    VoxelManager voxel_manager;
    VoxelChunk *test_cube = voxel_manager.generate_test_cube({0.0F, 0.0F, 0.0F});
    
    auto context = create_graphics_context();

    auto model = build_model(context, *test_cube);
    glm::mat3x4 transform = {
	1.0F, 0.0F, 0.0F, 0.0F,
	0.0F, 1.0F, 0.0F, 0.0F,
	0.0F, 0.0F, 1.0F, 0.0F
    };
    auto object = build_object(context, model, transform);
    auto scene = build_scene(context, {object});

    while (!should_exit(context)) {
	render_frame(context, scene);
    }
}
