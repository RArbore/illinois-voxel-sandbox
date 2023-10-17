#include <filesystem>
#include "AnvilLoader.h"

#include "enkimi.h"

AnvilLoader::AnvilLoader(const std::string &filePath) : filePath(filePath) {
    if (filePath.empty()) {
        throw std::runtime_error("File path is empty");
    }

    if (filePath.substr(filePath.find_last_of(".") + 1) != "mca") {
        throw std::runtime_error("File extension is not .mca");
    }

    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("File does not exist");
    }
}

void AnvilLoader::loadRegion() {
    FILE *file = fopen(filePath.c_str(), "rb");
    auto regionFile = enkiRegionFileLoad(file);

    for (auto i = 0; i < ENKI_MI_REGION_CHUNKS_NUMBER; i++) {
        enkiNBTDataStream stream;
        enkiInitNBTDataStreamForChunk(regionFile, i, &stream);
        if (stream.dataLength) {
            auto chunk = enkiNBTReadChunk(&stream);
            auto chunkPos = enkiGetChunkOrigin(&chunk);

            int64_t numVoxels = 0;
            for (auto section = 0; section < ENKI_MI_NUM_SECTIONS_PER_CHUNK; section++) {
                if (chunk.sections[section]) {
                    auto sectionPos = enkiGetChunkSectionOrigin(&chunk, section);
                    for (auto y = 0; y < ENKI_MI_SIZE_SECTIONS; y++) {
                        for (auto z = 0; z < ENKI_MI_SIZE_SECTIONS; z++) {
                            for (auto x = 0; x < ENKI_MI_SIZE_SECTIONS; x++) {
                                enkiMICoordinate pos{x, y, z};
                                auto voxel = enkiGetChunkSectionVoxel(&chunk, section, pos);
                                if (voxel) {
                                    // Generate voxel
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
