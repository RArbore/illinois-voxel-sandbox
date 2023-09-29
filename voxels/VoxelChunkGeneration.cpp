#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <string>
#include <random>
#include <cmath>

#include <external/glm/glm/glm.hpp>
#include <external/PerlinNoise.hpp>
#include <external/ogt_vox.h>

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
                std::byte alpha = static_cast<std::byte>(255);

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
    std::vector<std::byte> data(width * height * depth * 4,
                                static_cast<std::byte>(0));

    for (uint32_t x = 0; x < width; ++x) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t z = 0; z < depth; ++z) {
                if (densityfunction_({x, y, z}) > -30.0) {
                    std::byte red = static_cast<std::byte>(x * 15);
                    std::byte green = static_cast<std::byte>(y * 15);
                    std::byte blue = static_cast<std::byte>(z * 15);
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

double densityfunction_(const glm::vec3 &position) {
    const siv::PerlinNoise::seed_type seed = 123456u;
    const siv::PerlinNoise perlin1{seed};
    const siv::PerlinNoise perlin2{seed + 1};
    // const siv::PerlinNoise perlin3{ seed + 2 };
    const double noise1 =
        perlin1.noise3D(position.x * 0.01, position.y * 0.01,
                        position.z * 0.01); // scale them down for the noise
    const double noise2 =
        perlin2.noise3D(position.x * 0.1, position.y * 0.1, position.z * 0.1);
    double density =
        position.y - 40; // origin seems to be at top corner of chunk
    // density += std::sin(position.x/ 100) * 10;
    // density += std::cos(position.y / 100) * 15;
    density += noise1 * 5;
    density += noise2 * 10;
    return density;
}

std::vector<std::byte> generate_terrain(uint32_t width, uint32_t height,
                                        uint32_t depth, float density) {
    std::vector<std::byte> data(width * height * depth * 4,
                                static_cast<std::byte>(0));
    data = generate_basic_procedural_chunk(width, height, depth);
    // for loop depending on density
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xran(0, width - 1);
    std::uniform_int_distribution<> zran(0, depth - 1);
    // Find random place in chunk, find highest point, put tree there

    return data;
}

std::shared_ptr<GraphicsScene> test_loader(std::string filepath, ChunkManager chunk_manager, std::shared_ptr<GraphicsContext> context) {
    std::ifstream fstream(filepath, std::ios::in | std::ios::binary);
    std::vector<uint8_t> buffer(std::filesystem::file_size(filepath));
    fstream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    const ogt_vox_scene *voxscene = ogt_vox_read_scene(buffer.data(), buffer.size());

    std::cout << "Number of models: " << voxscene->num_models << std::endl;

    auto model = voxscene->models[0];

    std::cout << "X size: " << model->size_x << std::endl;


    std::vector<std::byte> data(model->size_z * model->size_y *
                                        model->size_x * 4,
                                        static_cast<std::byte>(0));
    uint32_t voxel_index = 0;
    for (uint32_t z = 0; z < model->size_z; z++) {
        for (uint32_t y = 0; y < model->size_y; y++) {
            for (uint32_t x = 0; x < model->size_x; x++, voxel_index++) {
                // if color index == 0, this voxel is empty, otherwise it is
                // solid.
                uint32_t color_index = model->voxel_data[voxel_index];
                // doing without color first
                // ogt_vox_rgba color = voxscene->palette.color[color_index];
                bool is_voxel_solid = (color_index != 0);
                if (is_voxel_solid) {
                    data[voxel_index * 4] = static_cast<std::byte>(255);
                    data[voxel_index * 4 + 1] = static_cast<std::byte>(255);
                    data[voxel_index * 4 + 2] = static_cast<std::byte>(255);
                    data[voxel_index * 4 + 3] = static_cast<std::byte>(255);
                } else {
                    data[voxel_index * 4] = static_cast<std::byte>(0);
                    data[voxel_index * 4 + 1] = static_cast<std::byte>(0);
                    data[voxel_index * 4 + 2] = static_cast<std::byte>(0);
                    data[voxel_index * 4 + 3] = static_cast<std::byte>(0);
                }
            }
        }
    }

    VoxelChunkPtr model_pointer = chunk_manager.add_chunk(
            std::move(data), model->size_x, model->size_y, model->size_z, VoxelChunk::Format::Raw,
            VoxelChunk::AttributeSet::Color);

    std::shared_ptr<GraphicsModel> graphicsmodel = build_model(context, model_pointer);

    std::cout << "Number of instances: " << voxscene->num_instances << std::endl;
    
    auto instance = voxscene->instances[0];

    ogt_vox_transform trans = instance.transform;
    glm::mat3x4 glmtransform = {trans.m00, trans.m10, trans.m20, trans.m30,
                                trans.m01, trans.m11, trans.m21, trans.m31, 
                                trans.m02, trans.m12, trans.m22, trans.m32};

    std::shared_ptr<GraphicsObject> obj = build_object(context, 
                                                    graphicsmodel,
                                                    glmtransform);

    std::shared_ptr<GraphicsScene> gscene = build_scene(context, {obj});

    ogt_vox_destroy_scene(voxscene);
    return gscene;
}

std::shared_ptr<GraphicsScene>
load_vox_scene(std::string filepath, ChunkManager chunk_manager, std::shared_ptr<GraphicsContext> context) {
    ASSERT(std::filesystem::exists(filepath), "Tried to load .vox scene from file that doesn't exist.");
    
    std::ifstream fstream(filepath, std::ios::in | std::ios::binary);
    std::vector<uint8_t> buffer(std::filesystem::file_size(filepath));
    fstream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    
    const ogt_vox_scene *voxscene = ogt_vox_read_scene(buffer.data(), buffer.size());

    std::vector<std::shared_ptr<GraphicsModel>> models;
    
    for (size_t i = 0; i < voxscene->num_models; ++i) {
        auto model = voxscene->models[i];
        std::vector<std::byte> data(model->size_z * model->size_y *
                                        model->size_x * 4,
                                        static_cast<std::byte>(0));
        uint32_t voxel_index = 0;
        for (uint32_t z = 0; z < model->size_z; z++) {
            for (uint32_t y = 0; y < model->size_y; y++) {
                for (uint32_t x = 0; x < model->size_x; x++, voxel_index++) {
                    // if color index == 0, this voxel is empty, otherwise it is
                    // solid.
                    uint32_t color_index = model->voxel_data[voxel_index];
                    // doing without color first
                    // ogt_vox_rgba color = voxscene->palette.color[color_index];
                    bool is_voxel_solid = (color_index != 0);
                    if (is_voxel_solid) {
                        data[voxel_index * 4] = static_cast<std::byte>(255);
                        data[voxel_index * 4 + 1] = static_cast<std::byte>(255);
                        data[voxel_index * 4 + 2] = static_cast<std::byte>(255);
                        data[voxel_index * 4 + 3] = static_cast<std::byte>(255);
                    }
                }
            }
        }
        VoxelChunkPtr model_pointer = chunk_manager.add_chunk(
            std::move(data), model->size_x, model->size_y, model->size_z, VoxelChunk::Format::Raw,
            VoxelChunk::AttributeSet::Color);
        std::shared_ptr<GraphicsModel> graphicsmodel = build_model(context, model_pointer);
        


        models.emplace_back(std::move(graphicsmodel));
    }

    std::vector<std::shared_ptr<GraphicsObject>> objs;
    for (size_t i = 0; i < voxscene->num_instances; ++i) {
        ogt_vox_instance instance = voxscene->instances[i];
        ogt_vox_transform trans = instance.transform;
        glm::mat3x4 glmtransform = {trans.m00, trans.m10, trans.m20, trans.m30,
                                    trans.m01, trans.m11, trans.m21, trans.m31, 
                                    trans.m02, trans.m12, trans.m22, trans.m32};
        std::shared_ptr<GraphicsObject> obj = build_object(context, 
                                                    models[instance.model_index],
                                                    glmtransform);

        objs.emplace_back(std::move(obj));
    }

    std::shared_ptr<GraphicsScene> gscene = build_scene(context, objs);
    
    ogt_vox_destroy_scene(voxscene);
    return gscene;
}
