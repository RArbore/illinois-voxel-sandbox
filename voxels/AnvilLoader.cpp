#include <filesystem>
#include "AnvilLoader.h"

#include "enkimi.h"
#include "Conversion.h"

AnvilLoader::AnvilLoader(const std::string &filePath, std::shared_ptr<GraphicsContext> context, const ChunkManager &chunkManager)
        : filePath(filePath), context(context), chunk_manager(chunkManager) {
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

std::vector<std::pair<glm::mat3x4, std::shared_ptr<GraphicsModel>>> AnvilLoader::build_models() {
    std::vector<std::pair<glm::mat3x4, std::shared_ptr<GraphicsModel>>> models;

    FILE *file = fopen(filePath.c_str(), "rb");
    auto regionFile = enkiRegionFileLoad(file);

    for (auto i = 0; i < ENKI_MI_REGION_CHUNKS_NUMBER; i++) {
        enkiNBTDataStream stream;
        enkiInitNBTDataStreamForChunk(regionFile, i, &stream);
        if (stream.dataLength) {
            auto chunk = enkiNBTReadChunk(&stream);
            auto chunkPos = enkiGetChunkOrigin(&chunk);

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
                                    std::vector<std::byte> data(16 * 16 * 16 * 4, static_cast<std::byte>(0));
                                    for (auto vx = 0; vx < 16; vx++) {
                                        for (auto vy = 0; vy < 16; vy++) {
                                            for (auto vz = 0; vz < 16; vz++) {
                                                size_t voxel_idx = x + y * 16 + z * 16 * 16;
                                                data.at(voxel_idx * 4) = static_cast<std::byte>(255);
                                                data.at(voxel_idx * 4 + 1) = static_cast<std::byte>(0);
                                                data.at(voxel_idx * 4 + 2) = static_cast<std::byte>(0);
                                                data.at(voxel_idx * 4 + 3) = static_cast<std::byte>(255);
                                            }
                                        }
                                    }

                                    auto svo = convert_raw_to_svo(data, 16, 16, 16, 4);
                                    auto chunk = chunk_manager.add_chunk(std::move(svo), 16, 16, 16,
                                                                         VoxelChunk::Format::SVO,
                                                                         VoxelChunk::AttributeSet::Color);
                                    glm::mat3x4 transform1 = {1.0F, 0.0F,   0.0F, (sectionPos.x + pos.x) * 16, 0.0F, 1.0F,
                                                              0.0F, (sectionPos.y + pos.y) * 16, 0.0F, 0.0F, 1.0F, (sectionPos.z + pos.z) * 16};
                                    models.push_back({transform1, build_model(context, chunk)});
                                }
                            }
                        }
                    }
                }
            }
        }
        enkiNBTFreeAllocations( &stream );
    }
    enkiRegionFileFreeAllocations( &regionFile );
    return models;
}

std::shared_ptr<GraphicsScene> AnvilLoader::build_scene() {
    auto models = build_models();
    std::cout << "INFO: Loaded " << models.size() << " models (" << models.size() * 16 * 16 * 16 << " voxels).\n";
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    for (auto &model : models) {
        objects.push_back(build_object(context, model.second, model.first));
    }
    return ::build_scene(context, objects);
}
