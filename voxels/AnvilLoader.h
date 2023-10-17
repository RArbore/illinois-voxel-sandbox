#ifndef ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H
#define ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H

#include <graphics/GraphicsContext.h>
#include <vector>

class AnvilLoader {
public:
    AnvilLoader(const std::string &filePath, std::shared_ptr<GraphicsContext> context, const ChunkManager &chunkManager);

    std::vector<std::pair<glm::mat3x4, std::shared_ptr<GraphicsModel>>> build_models();

    std::shared_ptr<GraphicsScene> build_scene();
private:
    std::string filePath;
    std::shared_ptr<GraphicsContext> context;
    ChunkManager chunk_manager;
};

#endif // ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H
