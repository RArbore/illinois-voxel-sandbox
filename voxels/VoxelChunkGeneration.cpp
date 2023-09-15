#include "VoxelManager.h"
#include "PerlinNoise.hpp"
#include <cmath>


RawVoxelChunk *generate_test_cube(const glm::vec3 &position) {
    RawVoxelChunk *new_chunk = new RawVoxelChunk(position, 11, 11, 11);
    RawVoxel voxel =
        RawVoxel{.red_ = 255, .green_ = 0, .blue_ = 255, .alpha_ = 255};

    // The corners will be written to multiple times, but this shouldn't affect
    // anything
    for (int z = 0; z < 11; z++) {
        new_chunk->write_voxel(0, 0, z, voxel);
        new_chunk->write_voxel(0, 10, z, voxel);
        new_chunk->write_voxel(10, 0, z, voxel);
        new_chunk->write_voxel(10, 10, z, voxel);
    }

    voxel = RawVoxel{.red_ = 0, .green_ = 255, .blue_ = 255, .alpha_ = 255};
    for (int y = 0; y < 11; y++) {
        new_chunk->write_voxel(0, y, 0, voxel);
        new_chunk->write_voxel(0, y, 10, voxel);
        new_chunk->write_voxel(10, y, 0, voxel);
        new_chunk->write_voxel(10, y, 10, voxel);
    }

    voxel = RawVoxel{.red_ = 255, .green_ = 255, .blue_ = 0, .alpha_ = 255};
    for (int x = 0; x < 11; x++) {
        new_chunk->write_voxel(x, 0, 0, voxel);
        new_chunk->write_voxel(x, 0, 10, voxel);
        new_chunk->write_voxel(x, 10, 0, voxel);
        new_chunk->write_voxel(x, 10, 10, voxel);
    }

    chunks_.push_back(new_chunk);

    return new_chunk;
}

RawVoxelChunk *generate_sphere_chunk(const glm::vec3 &position, const glm::vec3 &size, const int radius) {
    RawVoxelChunk *new_chunk = new RawVoxelChunk(position, (int)size.x, (int)size.y, (int)size.z);
    RawVoxel voxel =
        RawVoxel{.red_ = 255, .green_ = 255, .blue_ = 255, .alpha_ = 255};
    
    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            for (int z = 0; z < size.z; z++) {
                if (std::pow(x - size.x/2, 2) + std::pow(y - size.y/2, 2) + std::pow(z - size.z/2, 2) < std::pow(radius, 2)) {
                    uint8_t red = x * 4;
		    uint8_t green = y * 4;
		    uint8_t blue = z * 4;
                    voxel = RawVoxel{.red_ = red, .green_ = green, .blue_ = blue, .alpha_ = 255};
                    new_chunk->write_voxel(x, y, z, voxel);
                }
            }
        }
    }

    chunks_.push_back(new_chunk);

    return new_chunk;
}

RawVoxelChunk *generate_procedural_chunk(const glm::vec3 &position, const glm::vec3 &size) {
    RawVoxelChunk *new_chunk = new RawVoxelChunk(position, (int)size.x, (int)size.y, (int)size.z);
    RawVoxel voxel = RawVoxel{.red_ = 255, .green_ = 255, .blue_ = 255, .alpha_ = 255};
    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            for (int z = 0; z < size.z; z++) {
                
                if (densityfunction_({position.x + x, position.y + y, position.z + z}) > 0) {
                    uint8_t color = (y * -7 + 188);
                    // if (y > 40) {
                    //     voxel = RawVoxel{.red_ = color, .green_ = color, .blue_ = color, .alpha_ = 255};
                    // } else {
                    //     voxel = RawVoxel{.red_ = 139, .green_ = 69, .blue_ = 19, .alpha_ = 255};
                    // }
                    voxel = RawVoxel{.red_ = color, .green_ = color, .blue_ = color, .alpha_ = 255};
                    new_chunk->write_voxel(x, y, z, voxel);
                }
            }
        }
    }

    chunks_.push_back(new_chunk);

    return new_chunk;
}

double densityfunction_(const glm::vec3 &position) {

    const siv::PerlinNoise::seed_type seed = 123456u;
    const siv::PerlinNoise perlin1{ seed };
    const siv::PerlinNoise perlin2{ seed + 1 };
    // const siv::PerlinNoise perlin3{ seed + 2 };
    const double noise1 = perlin1.noise3D(position.x * 0.01, position.y * 0.01, position.z * 0.01); //scale them down for the noise
    const double noise2 = perlin2.noise3D(position.x * 0.1, position.y * 0.1, position.z * 0.1);
    double density = position.y - 40; //origin seems to be at top corner of chunk
    // density += std::sin(position.x/ 100) * 10;
    // density += std::cos(position.y / 100) * 15;
    density += noise1 * 5;
    density += noise2 * 10;
    return density;
}
