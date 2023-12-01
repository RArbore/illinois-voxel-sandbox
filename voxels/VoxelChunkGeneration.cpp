#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdio.h>
#include <string>

#include <external/FastNoiseLite.h>
#include <external/glm/glm/glm.hpp>
#include <external/ogt_vox.h>

#include "Conversion.h"
#include "VoxelChunkGeneration.h"
#include "utils/Assert.h"

// Have to include so that can read in vox files for now
#include <graphics/GraphicsContext.h>

std::vector<std::byte> generate_basic_sphere_chunk(uint32_t width,
                                                   uint32_t height,
                                                   uint32_t depth,
                                                   float radius) {
    std::vector<std::byte> data(width * height * depth * 4,
                                static_cast<std::byte>(0));

    for (uint32_t x = 0; x < width; ++x) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t z = 0; z < depth; ++z) {
                if (std::pow(static_cast<double>(x) - width / 2, 2) +
                        std::pow(static_cast<double>(y) - height / 2, 2) +
                        std::pow(static_cast<double>(z) - depth / 2, 2) <
                    std::pow(radius, 2)) {
                    std::byte red = static_cast<std::byte>(x * 4);
                    std::byte green = static_cast<std::byte>(y * 4);
                    std::byte blue = static_cast<std::byte>(z * 4);
                    std::byte alpha = static_cast<std::byte>(255);

                    size_t voxel_idx = x + y * width + z * width * height;
                    data.at(voxel_idx * 4) = red;
                    data.at(voxel_idx * 4 + 1) = green;
                    data.at(voxel_idx * 4 + 2) = blue;
                    data.at(voxel_idx * 4 + 3) = alpha;
                }
            }
        }
    }

    return data;
}

std::vector<std::byte>
generate_basic_filled_chunk(uint32_t width, uint32_t height, uint32_t depth) {
    std::vector<std::byte> data(width * height * depth * 4,
                                static_cast<std::byte>(0));

    for (uint32_t x = 0; x < width; ++x) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t z = 0; z < depth; ++z) {
                std::byte red = static_cast<std::byte>(x * 30);
                std::byte green = static_cast<std::byte>(y * 30);
                std::byte blue = static_cast<std::byte>(z * 30);
		        // Used in emissive format for intensity
                std::byte alpha = static_cast<std::byte>(50);

                size_t voxel_idx = x + y * width + z * width * height;
                data.at(voxel_idx * 4) = red;
                data.at(voxel_idx * 4 + 1) = green;
                data.at(voxel_idx * 4 + 2) = blue;
                data.at(voxel_idx * 4 + 3) = alpha;
            }
        }
    }

    return data;
}

std::vector<std::byte> generate_basic_procedural_chunk(uint32_t width,
                                                       uint32_t height,
                                                       uint32_t depth) {
    std::vector<std::byte> data(static_cast<size_t>(width) *
                                    static_cast<size_t>(height) *
                                    static_cast<size_t>(depth) * 4,
                                static_cast<std::byte>(0));
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    for (uint32_t x = 0; x < width; ++x) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t z = 0; z < depth; ++z) {
                if (noise.GetNoise(static_cast<float>(x), static_cast<float>(y),
                                   static_cast<float>(z)) > 0.0F) {
                    std::byte red = static_cast<std::byte>(255);
                    std::byte green = static_cast<std::byte>(255);
                    std::byte blue = static_cast<std::byte>(255);
                    std::byte alpha = static_cast<std::byte>(255);

                    size_t voxel_idx = x + y * width + z * width * height;
                    data.at(voxel_idx * 4) = red;
                    data.at(voxel_idx * 4 + 1) = green;
                    data.at(voxel_idx * 4 + 2) = blue;
                    data.at(voxel_idx * 4 + 3) = alpha;
                }
            }
        }
    }

    return data;
}

std::shared_ptr<GraphicsScene>
load_vox_scene(const std::string &filepath, ChunkManager &chunk_manager,
               std::shared_ptr<GraphicsContext> context) {
    ASSERT(std::filesystem::exists(filepath),
           "Tried to load .vox scene from file that doesn't exist.");

    std::ifstream fstream(filepath, std::ios::in | std::ios::binary);
    std::vector<uint8_t> buffer(std::filesystem::file_size(filepath));

    fstream.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

    const ogt_vox_scene *voxscene =
        ogt_vox_read_scene(buffer.data(), buffer.size());

    std::cout << "Number of models: " << voxscene->num_models << std::endl;
    ASSERT(voxscene->num_models > 0,
           "Tried to load .vox scene with no models.");

    std::vector<std::shared_ptr<GraphicsModel>> models;

    for (size_t i = 0; i < voxscene->num_models; ++i) {
        auto model = voxscene->models[i];

        std::cout << "Loading model: " << i << "\n";
        uint32_t width = model->size_x;
        uint32_t height = model->size_z;
        uint32_t depth = model->size_y;
        std::cout << "Width: " << width << "\n";
        std::cout << "Height: " << height << "\n";
        std::cout << "Depth: " << depth << "\n";

        std::vector<std::byte> data(width * height * depth * 4,
                                    static_cast<std::byte>(0));
        uint32_t output_index = 0;
        for (uint32_t y = 0; y < model->size_y; y++) {
            for (uint32_t z = 0; z < model->size_z; z++) {
                for (uint32_t x = 0; x < model->size_x; x++, output_index++) {
                    uint32_t color_index =
                        model->voxel_data[(model->size_x - x - 1) +
                                          (model->size_y - y - 1) *
                                              model->size_x +
                                          (model->size_z - z - 1) *
                                              model->size_x * model->size_y];
                    ogt_vox_rgba color = voxscene->palette.color[color_index];
                    if (color_index != 0) {
                        data[output_index * 4] =
                            static_cast<std::byte>(color.r);
                        data[output_index * 4 + 1] =
                            static_cast<std::byte>(color.g);
                        data[output_index * 4 + 2] =
                            static_cast<std::byte>(color.b);
                        data[output_index * 4 + 3] =
                            static_cast<std::byte>(color.a);
                    }
                }
            }
        }
        VoxelChunkPtr model_pointer = chunk_manager.add_chunk(
            std::move(data), width, height, depth, VoxelChunk::Format::Raw,
            VoxelChunk::AttributeSet::Color);
        std::shared_ptr<GraphicsModel> graphicsmodel =
            build_model(context, model_pointer);

        models.emplace_back(std::move(graphicsmodel));
    }
    std::cout << "Number of instances: " << voxscene->num_instances
              << std::endl;
    std::vector<std::shared_ptr<GraphicsObject>> objs;
    for (size_t i = 0; i < voxscene->num_instances; ++i) {
        ogt_vox_instance instance = voxscene->instances[i];
        ogt_vox_transform trans = instance.transform;
        glm::mat3x4 glmtransform = {trans.m00, trans.m02, trans.m01, trans.m03,
                                    trans.m20, trans.m22, trans.m21, trans.m23,
                                    trans.m10, trans.m12, trans.m11, trans.m13};
        std::shared_ptr<GraphicsObject> obj =
            build_object(context, models[instance.model_index], glmtransform);

        objs.emplace_back(std::move(obj));
    }

    std::shared_ptr<GraphicsScene> gscene = build_scene(context, objs);

    ogt_vox_destroy_scene(voxscene);
    return gscene;
}
