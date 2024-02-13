#include <filesystem>
#include <cstring>
#include <cmath>

#include <external/tinyobjloader/tiny_obj_loader.h>
#include <external/glm/glm/glm.hpp>
#include <external/stb_image.h>

#include "Voxelize.h"
#include "Conversion.h"
#include "utils/Assert.h"

struct Triangle {
    glm::vec3 a, b, c;
    glm::vec2 t_a, t_b, t_c;

    float min_x() const {
    return fmin(a.x, fmin(b.x, c.x));
    }

    float max_x() const {
    return fmax(a.x, fmax(b.x, c.x));
    }

    float min_y() const {
    return fmin(a.y, fmin(b.y, c.y));
    }

    float max_y() const {
    return fmax(a.y, fmax(b.y, c.y));
    }

    float min_z() const {
    return fmin(a.z, fmin(b.z, c.z));
    }

    float max_z() const {
    return fmax(a.z, fmax(b.z, c.z));
    }
};

bool tri_aabb_sat(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 aabb, glm::vec3 axis) {
    float p0 = glm::dot(v0, axis);
    float p1 = glm::dot(v1, axis);
    float p2 = glm::dot(v2, axis);

    float r = aabb.x * glm::abs(glm::dot(glm::vec3(1, 0, 0), axis)) +
        aabb.y * glm::abs(glm::dot(glm::vec3(0, 1, 0), axis)) +
        aabb.z * glm::abs(glm::dot(glm::vec3(0, 0, 1), axis));

    float maxP = glm::max(p0, glm::max(p1, p2));
    float minP = glm::min(p0, glm::min(p1, p2));

    return !(glm::max(-maxP, minP) > r);
}

bool tri_aabb(const Triangle &triangle, glm::vec3 aabb_center, glm::vec3 aabb_extents) {
    Triangle adj_triangle = triangle;
    adj_triangle.a -= aabb_center;
    adj_triangle.b -= aabb_center;
    adj_triangle.c -= aabb_center;

    glm::vec3 ab = glm::normalize(adj_triangle.b - adj_triangle.a);
    glm::vec3 bc = glm::normalize(adj_triangle.c - adj_triangle.b);
    glm::vec3 ca = glm::normalize(adj_triangle.a - adj_triangle.c);

    glm::vec3 a00 = glm::vec3(0.0, -ab.z, ab.y);
    glm::vec3 a01 = glm::vec3(0.0, -bc.z, bc.y);
    glm::vec3 a02 = glm::vec3(0.0, -ca.z, ca.y);

    glm::vec3 a10 = glm::vec3(ab.z, 0.0, -ab.x);
    glm::vec3 a11 = glm::vec3(bc.z, 0.0, -bc.x);
    glm::vec3 a12 = glm::vec3(ca.z, 0.0, -ca.x);

    glm::vec3 a20 = glm::vec3(-ab.y, ab.x, 0.0);
    glm::vec3 a21 = glm::vec3(-bc.y, bc.x, 0.0);
    glm::vec3 a22 = glm::vec3(-ca.y, ca.x, 0.0);

    if (
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a00) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a01) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a02) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a10) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a11) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a12) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a20) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a21) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, a22) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, glm::vec3(1, 0, 0)) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, glm::vec3(0, 1, 0)) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, glm::vec3(0, 0, 1)) ||
        !tri_aabb_sat(adj_triangle.a, adj_triangle.b, adj_triangle.c, aabb_extents, glm::cross(ab, bc))
    )
    {
        return false;
    }

    return true;
}

std::vector<std::byte> raw_voxelize_obj(std::string_view filepath, float voxel_size, uint32_t &out_chunk_width, uint32_t &out_chunk_height, uint32_t &out_chunk_depth) {
    std::cout << filepath << " " << voxel_size << "\n";

    std::string inputfile(filepath);
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;
    
    ASSERT(reader.ParseFromFile(inputfile, reader_config), reader.Error());
    
    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();
    auto &materials = reader.GetMaterials();

    std::string directory(filepath);
    const auto last_slash = directory.find_last_of("/");
    if (last_slash == std::string::npos) {
        directory = "";
    } else {
        directory = directory.substr(0, last_slash + 1);
    }
    
    glm::vec3 min(0.0f, 0.0f, 0.0f), max(0.0f, 0.0f, 0.0f);
    for (int64_t i = 0; i < attrib.vertices.size(); i += 3) {
        min.x = fmin(min.x, attrib.vertices[i]);
        min.y = fmin(min.y, attrib.vertices[i + 1]);
        min.z = fmin(min.z, attrib.vertices[i + 2]);
        max.x = fmax(max.x, attrib.vertices[i]);
        max.y = fmax(max.y, attrib.vertices[i + 1]);
        max.z = fmax(max.z, attrib.vertices[i + 2]);
    }
    std::cout << "Bottom corner: (" << min.x << ", " << min.y << ", " << min.z << ")\n";
    std::cout << "Top corner: (" << max.x << ", " << max.y << ", " << max.z << ")\n";

    uint32_t chunk_width = static_cast<uint32_t>(ceil((max.x - min.x) / voxel_size));
    uint32_t chunk_height = static_cast<uint32_t>(ceil((max.y - min.y) / voxel_size));
    uint32_t chunk_depth = static_cast<uint32_t>(ceil((max.z - min.z) / voxel_size));
    std::cout << "Chunk size: (" << chunk_width << ", " << chunk_height << ", " << chunk_depth << ")\n";

    std::unordered_map<int, std::tuple<stbi_uc *, int, int>> loaded_textures;
    std::vector<std::byte> data(static_cast<size_t>(chunk_width) *
                                    static_cast<size_t>(chunk_height) *
                                    static_cast<size_t>(chunk_depth) * 4,
                                static_cast<std::byte>(0));

    for (const auto &shape : shapes) {
        std::cout << shape.mesh.indices.size() / 3 << " triangles\n";
        uint32_t num_printed = 0;
        for (int64_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            const auto &vert_a = shape.mesh.indices[i];
            const auto &vert_b = shape.mesh.indices[i + 1];
            const auto &vert_c = shape.mesh.indices[i + 2];

            auto index_a = vert_a.vertex_index;
            auto index_b = vert_b.vertex_index;
            auto index_c = vert_c.vertex_index;

            auto tex_index_a = vert_a.texcoord_index;
            auto tex_index_b = vert_b.texcoord_index;
            auto tex_index_c = vert_c.texcoord_index;

            bool has_materials =
                (tex_index_a >= 0 && tex_index_b >= 0 && tex_index_c >= 0);

            const Triangle tri(
                glm::vec3(attrib.vertices.at(3 * index_a),
                          attrib.vertices.at(3 * index_a + 1),
                          attrib.vertices.at(3 * index_a + 2)),
                glm::vec3(attrib.vertices.at(3 * index_b),
                          attrib.vertices.at(3 * index_b + 1),
                          attrib.vertices.at(3 * index_b + 2)),
                glm::vec3(attrib.vertices.at(3 * index_c),
                          attrib.vertices.at(3 * index_c + 1),
                          attrib.vertices.at(3 * index_c + 2)),
                has_materials ? glm::vec2(attrib.texcoords.at(2 * tex_index_a),
                                          attrib.texcoords.at(2 * tex_index_a + 1)) : glm::vec2(0.0f),
                has_materials ? glm::vec2(attrib.texcoords.at(2 * tex_index_b),
                                          attrib.texcoords.at(2 * tex_index_b + 1)) : glm::vec2(0.0f),
                has_materials ? glm::vec2(attrib.texcoords.at(2 * tex_index_c),
                                          attrib.texcoords.at(2 * tex_index_c + 1)) : glm::vec2(0.0f));

            glm::mat3 tri_matrix(tri.a, tri.b, tri.c);
            glm::mat3 inverse_tri_matrix = glm::inverse(tri_matrix);
            glm::vec3 unit_plane =
                glm::normalize(glm::cross(tri.a - tri.c, tri.b - tri.c));
            if (!std::isfinite(inverse_tri_matrix[0][0]) ||
                !std::isfinite(inverse_tri_matrix[0][1]) ||
                !std::isfinite(inverse_tri_matrix[0][2]) ||
                !std::isfinite(inverse_tri_matrix[1][0]) ||
                !std::isfinite(inverse_tri_matrix[1][1]) ||
                !std::isfinite(inverse_tri_matrix[1][2]) ||
                !std::isfinite(inverse_tri_matrix[2][0]) ||
                !std::isfinite(inverse_tri_matrix[2][1]) ||
                !std::isfinite(inverse_tri_matrix[2][2])) {
                continue;
            }

            int texture_width, texture_height;
            stbi_uc *texture_pixels;
            if (has_materials) {
                int face_mat_id = shape.mesh.material_ids[i / 3];
                if (loaded_textures.contains(face_mat_id)) {
                    auto texture = loaded_textures[face_mat_id];
                    texture_pixels = std::get<0>(texture);
                    texture_width = std::get<1>(texture);
                    texture_height = std::get<2>(texture);
                } else {
                    const tinyobj::material_t &mat = materials[face_mat_id];
                    const std::string &diffuse_texname = mat.diffuse_texname;
		    if (diffuse_texname.empty()) {
			has_materials = false;
		    } else {
			int channels;
			texture_pixels = stbi_load(
						   (directory + diffuse_texname).c_str(), &texture_width,
						   &texture_height, &channels, STBI_rgb_alpha);
			if (!texture_pixels) {
			    std::stringstream ss;
			    ss << "Couldn't load texture from the following path: ";
			    ss << (directory + diffuse_texname);
			    ss << "\n";
			    ASSERT(false, ss.str());
			}
			loaded_textures[face_mat_id] = {
			    texture_pixels, texture_width, texture_height};
		    }
                }
            }

            uint32_t min_voxel_x = static_cast<uint32_t>(
                floor(((tri.min_x() - min.x) / (max.x - min.x)) *
                      static_cast<float>(chunk_width)));
            uint32_t min_voxel_y = static_cast<uint32_t>(
                floor(((tri.min_y() - min.y) / (max.y - min.y)) *
                      static_cast<float>(chunk_height)));
            uint32_t min_voxel_z = static_cast<uint32_t>(
                floor(((tri.min_z() - min.z) / (max.z - min.z)) *
                      static_cast<float>(chunk_depth)));
            uint32_t max_voxel_x = static_cast<uint32_t>(
                ceil(((tri.max_x() - min.x) / (max.x - min.x)) *
                     static_cast<float>(chunk_width)));
            uint32_t max_voxel_y = static_cast<uint32_t>(
                ceil(((tri.max_y() - min.y) / (max.y - min.y)) *
                     static_cast<float>(chunk_height)));
            uint32_t max_voxel_z = static_cast<uint32_t>(
                ceil(((tri.max_z() - min.z) / (max.z - min.z)) *
                     static_cast<float>(chunk_depth)));
            if (max_voxel_x >= chunk_width) {
                max_voxel_x = chunk_width - 1;
            }
            if (max_voxel_y >= chunk_height) {
                max_voxel_y = chunk_height - 1;
            }
            if (max_voxel_z >= chunk_depth) {
                max_voxel_z = chunk_depth - 1;
            }

            for (uint32_t x = min_voxel_x; x <= max_voxel_x; ++x) {
                for (uint32_t y = min_voxel_y; y <= max_voxel_y; ++y) {
                    for (uint32_t z = min_voxel_z; z <= max_voxel_z; ++z) {
                        float tri_voxel_x = static_cast<float>(x) /
                                                chunk_width * (max.x - min.x) +
                                            min.x;
                        float tri_voxel_y = static_cast<float>(y) /
                                                chunk_height * (max.y - min.y) +
                                            min.y;
                        float tri_voxel_z = static_cast<float>(z) /
                                                chunk_depth * (max.z - min.z) +
                                            min.z;
                        if (tri_aabb(tri,
                                     glm::vec3(tri_voxel_x, tri_voxel_y,
                                               tri_voxel_z),
                                     glm::vec3(voxel_size))) {
                            std::byte r = static_cast<std::byte>(255),
				g = static_cast<std::byte>(255),
				b = static_cast<std::byte>(255),
				a = static_cast<std::byte>(255);
                            glm::vec3 plane_point = glm::vec3(
                                tri_voxel_x, tri_voxel_y, tri_voxel_z);
                            plane_point -=
                                unit_plane *
                                (glm::dot(unit_plane, plane_point - tri.a));
                            glm::vec3 barycentric_coords =
                                inverse_tri_matrix * plane_point;

                            if (has_materials) {
                                glm::vec2 uv = tri.t_a * barycentric_coords.x +
                                               tri.t_b * barycentric_coords.y +
                                               tri.t_c * barycentric_coords.z;
                                int tex_x =
                                    static_cast<int>(uv.x * texture_width);
                                int tex_y =
                                    static_cast<int>(uv.y * texture_height);
                                while (tex_x < 0) {
                                    tex_x += texture_width;
                                }
                                while (tex_x >= texture_width) {
                                    tex_x -= texture_width;
                                }
                                while (tex_y < 0) {
                                    tex_y += texture_height;
                                }
                                while (tex_y >= texture_height) {
                                    tex_y -= texture_height;
                                }
                                int linear_coord =
                                    tex_x + tex_y * texture_width;
                                size_t voxel_idx =
                                    x + (chunk_height - y - 1) * chunk_width +
                                    z * chunk_width * chunk_height;
                                if (texture_pixels[linear_coord * 4 + 3]) {
                                    r = static_cast<std::byte>(
                                        texture_pixels[linear_coord * 4]);
                                    g = static_cast<std::byte>(
                                        texture_pixels[linear_coord * 4 + 1]);
                                    b = static_cast<std::byte>(
                                        texture_pixels[linear_coord * 4 + 2]);
                                    a = static_cast<std::byte>(255);
                                } else {
                                    r = static_cast<std::byte>(0);
                                    g = static_cast<std::byte>(0);
                                    b = static_cast<std::byte>(0);
                                    a = static_cast<std::byte>(0);
				                }
                            }

                            size_t voxel_idx =
                                x + (chunk_height - y - 1) * chunk_width +
                                z * chunk_width * chunk_height;
                            data.at(voxel_idx * 4) = r;
                            data.at(voxel_idx * 4 + 1) = g;
                            data.at(voxel_idx * 4 + 2) = b;
                            data.at(voxel_idx * 4 + 3) = a;
                        }
                    }
                }
            }

            if (i * 100 / shape.mesh.indices.size() - num_printed >= 10) {
                num_printed = i * 100 / shape.mesh.indices.size();
                std::cout << num_printed << "% finished.\n";
            }
        }
    }
    std::cout << "100% finished.\n";

    for (auto [_, tex] : loaded_textures) {
        stbi_image_free(std::get<0>(tex));
    }

    out_chunk_width = chunk_width;
    out_chunk_height = chunk_height;
    out_chunk_depth = chunk_depth;
    return data;
}

uint32_t Voxelizer::at(uint32_t x, uint32_t y, uint32_t z) {
    static_assert(4 * sizeof(std::byte) == sizeof(uint32_t));
    if (x >= width_ || y >= height_ || z >= depth_) {
	return 0;
    }
    size_t linear_index = x + width_ * y + width_ * height_ * z;
    uint32_t voxel;
    memcpy(&voxel, voxels_.data() + 4 * linear_index, sizeof(uint32_t));
    return voxel;
}
