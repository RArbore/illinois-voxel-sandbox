#include "AnvilLoader.h"
#include "utils/anvil_id_list.h"
#include <filesystem>

#include "Conversion.h"
#include "enkimi.h"

#include "external/stb_image.h"

AnvilLoader::AnvilLoader(const std::string &filePath,
                         std::shared_ptr<GraphicsContext> context,
                         std::shared_ptr<ChunkManager> chunkManager,
                         std::string _texturePack)
    : filePath(filePath), context(context), chunk_manager(chunkManager),
      texturePack(_texturePack) {
    if (filePath.empty()) {
        throw std::runtime_error("File path is empty");
    }

    if (filePath.substr(filePath.find_last_of(".") + 1) != "mca") {
        throw std::runtime_error("File extension is not .mca");
    }

    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("File does not exist");
    }

    if (!std::filesystem::is_regular_file(filePath)) {
        throw std::runtime_error("File is not a regular file");
    }

    if (texturePack.empty()) {
        throw std::runtime_error("Texture pack is empty");
    }

    this->texturePack = texturePack + "/assets/minecraft/textures/block";
}

std::vector<std::pair<glm::mat3x4, std::shared_ptr<GraphicsModel>>>
AnvilLoader::build_models() {
    std::vector<std::pair<glm::mat3x4, std::shared_ptr<GraphicsModel>>> models;

    FILE *file = fopen(filePath.c_str(), "rb");
    auto regionFile = enkiRegionFileLoad(file);

    for (auto i = 0; i < ENKI_MI_REGION_CHUNKS_NUMBER; i++) {
        enkiNBTDataStream stream;
        enkiInitNBTDataStreamForChunk(regionFile, i, &stream);
        if (stream.dataLength) {
            auto chunk = enkiNBTReadChunk(&stream);
            auto chunkPos = enkiGetChunkOrigin(&chunk);

            for (auto section = 0; section < ENKI_MI_NUM_SECTIONS_PER_CHUNK;
                 section++) {
                if (chunk.sections[section]) {
                    auto sectionPos =
                        enkiGetChunkSectionOrigin(&chunk, section);
                    for (auto y = 0; y < ENKI_MI_SIZE_SECTIONS; y++) {
                        for (auto z = 0; z < ENKI_MI_SIZE_SECTIONS; z++) {
                            for (auto x = 0; x < ENKI_MI_SIZE_SECTIONS; x++) {
                                enkiMICoordinate pos{x, y, z};
                                auto voxel = enkiGetChunkSectionVoxelData(
                                    &chunk, section, pos);
                                std::string voxel_id =
                                    std::to_string(voxel.blockID) + ":" +
                                    std::to_string(voxel.dataValue);
                                if (voxel.blockID) {
                                    glm::mat3x4 transform1 = {
                                        1.0F, 0.0F,
                                        0.0F, (sectionPos.x + pos.x) * 16,
                                        0.0F, 1.0F,
                                        0.0F, (sectionPos.y + pos.y) * 16,
                                        0.0F, 0.0F,
                                        1.0F, (sectionPos.z + pos.z) * 16};
//                                    std::cout
//                                        << "INFO: Loading voxel (x:"
//                                        << (sectionPos.x + pos.x) * 16
//                                        << ", y:" << (sectionPos.y + pos.y) * 16
//                                        << ", z:" << (sectionPos.z + pos.z) * 16
//                                        << ") " << voxel_id << "\n";
                                    models.emplace_back(transform1,
                                                        get_model(voxel_id));
                                }
                            }
                        }
                    }
                }
            }
        }
        enkiNBTFreeAllocations(&stream);
    }
    enkiRegionFileFreeAllocations(&regionFile);
    return models;
}

std::shared_ptr<GraphicsScene> AnvilLoader::build_scene() {
    auto models = build_models();
    std::cout << "INFO: Loaded " << models.size() << " models ("
              << models.size() * 16 * 16 * 16 << " voxels).\n";
    std::vector<std::shared_ptr<GraphicsObject>> objects;
    for (auto &model : models) {
        objects.push_back(build_object(context, model.second, model.first));
    }
    return ::build_scene(context, objects);
}

std::shared_ptr<GraphicsModel>
AnvilLoader::get_model(const std::string &model_id) {
    if (model_map.find(model_id) == model_map.end()) {
        int block_id = std::stoi(model_id.substr(0, model_id.find(":")));
        int data_value = std::stoi(model_id.substr(model_id.find(":") + 1));
        auto text_type = anvilGetTypeNameByBlockId(block_id, data_value);
        std::cout << "INFO: Loading model " << text_type << "\n";

        bool hasTexture = false;
        auto texPath = texturePack + "/" + text_type + ".png";
        if (text_type != "") {
            if (std::filesystem::exists(texPath)) {
                hasTexture = true;
            }
        }

        int w;
        int h;
        int comp;

        std::vector<std::byte> data(16 * 16 * 16 * 4,
                                    static_cast<std::byte>(0));
        if (hasTexture) {
            auto tex = stbi_load((texPath).c_str(), &w, &h, &comp, STBI_rgb_alpha);

            for (auto vx = 0; vx < 16; vx++) {
                for (auto vy = 0; vy < 16; vy++) {
                    for (auto vz = 0; vz < 16; vz++) {
                        size_t voxel_idx = vx + vy * 16 + vz * 16 * 16;

                        // check if voxel is on the edge of the chunk
                        if (vx == 0 || vy == 0 || vz == 0 || vx == 15 ||
                            vy == 15 || vz == 15) {
                            int tx = 0;
                            int ty = 0;
                            if (vx == 0) {
                                tx = vz;
                                ty = vy;
                            } else if (vx == 15) {
                                tx = vz;
                                ty = vy;
                            } else if (vy == 0) {
                                tx = vz;
                                ty = vx;
                            } else if (vy == 15) {
                                tx = vz;
                                ty = vx;
                            } else if (vz == 0) {
                                tx = vx;
                                ty = vy;
                            } else if (vz == 15) {
                                tx = vx;
                                ty = vy;
                            }
                            // get color from tex
                            auto r = tex[4 * (tx + ty * 16) + 0];
                            auto g = tex[4 * (tx + ty * 16) + 1];
                            auto b = tex[4 * (tx + ty * 16) + 2];
                            auto a = tex[4 * (tx + ty * 16) + 3];
                            data.at(voxel_idx * 4) = static_cast<std::byte>(r);
                            data.at(voxel_idx * 4 + 1) =
                                static_cast<std::byte>(g);
                            data.at(voxel_idx * 4 + 2) =
                                static_cast<std::byte>(b);
                            data.at(voxel_idx * 4 + 3) =
                                static_cast<std::byte>(a);
                        }
                    }
                }
            }
        }

        auto chunk = chunk_manager->add_chunk(std::move(data), 16, 16, 16,
                                              VoxelChunk::Format::Raw,
                                              VoxelChunk::AttributeSet::Color);

        auto model = build_model(context, chunk);
        model_map[model_id] = model;
    }
    return model_map[model_id];
}
