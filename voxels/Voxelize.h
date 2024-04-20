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
