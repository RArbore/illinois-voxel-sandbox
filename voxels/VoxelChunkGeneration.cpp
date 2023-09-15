#include <cmath>

#include <external/glm/glm/glm.hpp>

#include "VoxelChunkGeneration.h"
#include "PerlinNoise.hpp"

std::vector<std::byte> generate_basic_sphere_chunk(uint32_t width, uint32_t height, uint32_t depth, float radius) {
    std::vector<std::byte> data(width * height * depth * 4, static_cast<std::byte>(0));
    
    for (uint32_t x = 0; x < width; ++x) {
	for (uint32_t y = 0; y < height; ++y) {
	    for (uint32_t z = 0; z < depth; ++z) {
                if (std::pow(static_cast<double>(x) - width / 2, 2) + std::pow(static_cast<double>(y) - height / 2, 2) + std::pow(static_cast<double>(z) - depth / 2, 2) < std::pow(radius, 2)) {
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

std::vector<std::byte> generate_basic_filled_chunk(uint32_t width, uint32_t height, uint32_t depth) {
    std::vector<std::byte> data(width * height * depth * 4, static_cast<std::byte>(0));
    
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

std::vector<std::byte> generate_basic_procedural_chunk(uint32_t width, uint32_t height, uint32_t depth) {
    std::vector<std::byte> data(width * height * depth * 4, static_cast<std::byte>(0));

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
