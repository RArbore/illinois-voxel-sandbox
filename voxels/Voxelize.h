#pragma once

#include <filesystem>
#include <optional>

#include <external/glm/glm/glm.hpp>
#include <external/stb_image.h>

#include "Voxel.h"

std::vector<std::byte> raw_voxelize_obj(std::string_view filepath,
                                        float voxel_size,
                                        uint32_t &out_chunk_width,
                                        uint32_t &out_chunk_height,
                                        uint32_t &out_chunk_depth);

struct Triangle {
    glm::vec3 a, b, c;
    glm::vec2 t_a, t_b, t_c;
    int face_mat_id;
    bool has_texture;

    float min_x() const { return fmin(a.x, fmin(b.x, c.x)); }

    float max_x() const { return fmax(a.x, fmax(b.x, c.x)); }

    float min_y() const { return fmin(a.y, fmin(b.y, c.y)); }

    float max_y() const { return fmax(a.y, fmax(b.y, c.y)); }

    float min_z() const { return fmin(a.z, fmin(b.z, c.z)); }

    float max_z() const { return fmax(a.z, fmax(b.z, c.z)); }

    bool tri_aabb(glm::vec3 aabb_center, glm::vec3 aabb_extents) const;

    bool tri_aabb_sat(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 aabb,
                      glm::vec3 axis) const;
};

class Model {
public: 
    Model() = default;
    Model(std::string_view filepath);
    ~Model();

    glm::vec3 get_max() const {return max_;}
    glm::vec3 get_min() const {return min_;}
    bool has_materials() const {return has_materials_;}

    const std::vector<Triangle> get_triangles() const {return triangles_;}
    const std::unordered_map<int, std::tuple<stbi_uc *, int, int>> &
    get_textures() const {return loaded_textures_;}

private:
    glm::vec3 min_, max_;
    std::vector<Triangle> triangles_;

    bool has_materials_;
    std::unordered_map<int, std::tuple<stbi_uc *, int, int>> loaded_textures_;
};

class Voxelizer {
  private:
    uint32_t width_, height_, depth_;
    uint32_t chunks_width_, chunks_height_, chunks_depth_;
    float voxel_size_;

    const bool actually_write_to_disk_ = false;

    // The max size (in bytes) that the voxelizer should store
    // before it should start storing older chunks on the disk.
    const uint64_t max_memory_usage_ = (uint64_t) 1 << 33;

    // The dimension size of each voxel chunk (e.g. N x N x N)
    const uint64_t voxel_chunk_size_ = (uint64_t) 1 << 10;

    // This only keeps track of the memory used by the voxel data itself,
    // and not the rest of the program.
    uint64_t current_memory_usage_ = 0;

    typedef std::vector<std::byte> VoxelizedChunk;
    std::vector<VoxelizedChunk> voxel_chunks_;

    VoxelizedChunk *last_chunk_ = nullptr;
    uint32_t last_chunk_x_ = ~0;
    uint32_t last_chunk_y_ = ~0;
    uint32_t last_chunk_z_ = ~0;

    Model model_;

    inline uint32_t linearize_chunk_index(uint32_t x, uint32_t y, uint32_t z) const;

    inline std::tuple<uint32_t, uint32_t, uint32_t> get_voxel_chunk_index(uint32_t x, uint32_t y, uint32_t z) const;
    void voxelize_chunk(uint32_t chunk_x, uint32_t chunk_y, uint32_t chunk_z);

    // The chunk to voxelize should not be written out to disk
    std::optional<uint32_t> find_old_voxel_chunk(uint32_t chunk_to_voxelize) const;
    void write_voxels_to_disk(uint32_t chunk_index);
    void read_voxels_from_disk(uint32_t chunk_index);
    std::filesystem::path voxels_directory_;

  public:

    Voxelizer(std::string_view filepath,
              std::tuple<uint32_t, uint32_t, uint32_t> size);
    ~Voxelizer();

    uint32_t at(uint32_t x, uint32_t y, uint32_t z);
};
