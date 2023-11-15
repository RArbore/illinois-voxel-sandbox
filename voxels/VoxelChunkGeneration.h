#pragma once

#include "Voxel.h"
#include <graphics/GraphicsContext.h>

std::vector<std::byte> generate_basic_sphere_chunk(uint32_t width,
                                                   uint32_t height,
                                                   uint32_t depth,
                                                   float radius);
std::vector<std::byte>
generate_basic_filled_chunk(uint32_t width, uint32_t height, uint32_t depth);
std::vector<std::byte> generate_basic_procedural_chunk(uint32_t width,
                                                       uint32_t height,
                                                       uint32_t depth);

std::shared_ptr<GraphicsScene>
load_vox_scene(const std::string &filepath, ChunkManager &chunk_manager,
               std::shared_ptr<GraphicsContext> context);
std::vector<std::byte>
generate_island_chunk(uint32_t density, uint32_t radius, uint32_t depth);
std::vector<std::byte> generate_procedural_rock(uint32_t density,
                                                 uint32_t radius,
                                                 uint32_t depth,
                                                 uint32_t scale);

class BabyChunk {
    public:
        BabyChunk(uint32_t width, uint32_t height, uint32_t depth);
        std::vector<std::byte> get_data();
        uint32_t get_width() const;
        uint32_t get_height() const;
        uint32_t get_depth() const;
        void write_voxel(uint32_t x, uint32_t y, uint32_t z, uint32_t r,
                         uint32_t g, uint32_t b, uint32_t a);
        bool get_voxel(uint32_t x, uint32_t y, uint32_t z) const;
    private:
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        std::vector<std::byte> data;
};
