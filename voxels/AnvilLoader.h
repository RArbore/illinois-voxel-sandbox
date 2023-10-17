#ifndef ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H
#define ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H

#include <graphics/GraphicsContext.h>
#include <vector>

class AnvilLoader {
public:
    explicit AnvilLoader(const std::string &filePath);

    std::vector<GraphicsModel> build_models();
private:
    void loadRegion();

    std::string filePath;
};

#endif // ILLINOIS_VOXEL_SANDBOX_ANVILLOADER_H
