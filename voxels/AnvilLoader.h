#ifndef ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H
#define ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H

#include <graphics/GraphicsContext.h>
#include <map>
#include <vector>

class AnvilLoader {
public:
    AnvilLoader(const std::string &filePath, std::shared_ptr<GraphicsContext> context,
              std::shared_ptr<ChunkManager> chunkManager, std::string texturePack);

    std::vector<std::pair<glm::mat3x4, std::shared_ptr<GraphicsModel>>> build_models();

    std::shared_ptr<GraphicsScene> build_scene();

    std::shared_ptr<GraphicsModel> get_model(const std::string &model_id);
private:
    std::string filePath;
    std::shared_ptr<GraphicsContext> context;
    std::shared_ptr<ChunkManager> chunk_manager;
    std::map<std::string, std::shared_ptr<GraphicsModel>> model_map;
    std::string texturePack;
};

#endif // ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H
