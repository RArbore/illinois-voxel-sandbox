#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <external/tinyobjloader/tiny_obj_loader.h>

#include "Conversion.h"
#include "Voxelize.h"
#include "utils/Assert.h"

std::vector<std::byte> raw_voxelize_obj(std::string_view filepath,
                                        float voxel_size,
                                        uint32_t &out_chunk_width,
                                        uint32_t &out_chunk_height,
                                        uint32_t &out_chunk_depth) {
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
    std::cout << "Bottom corner: (" << min.x << ", " << min.y << ", " << min.z
              << ")\n";
    std::cout << "Top corner: (" << max.x << ", " << max.y << ", " << max.z
              << ")\n";

    uint32_t chunk_width =
        static_cast<uint32_t>(ceil((max.x - min.x) / voxel_size));
    uint32_t chunk_height =
        static_cast<uint32_t>(ceil((max.y - min.y) / voxel_size));
    uint32_t chunk_depth =
        static_cast<uint32_t>(ceil((max.z - min.z) / voxel_size));
    std::cout << "Chunk size: (" << chunk_width << ", " << chunk_height << ", "
              << chunk_depth << ")\n";

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
                has_materials
                    ? glm::vec2(attrib.texcoords.at(2 * tex_index_a),
                                attrib.texcoords.at(2 * tex_index_a + 1))
                    : glm::vec2(0.0f),
                has_materials
                    ? glm::vec2(attrib.texcoords.at(2 * tex_index_b),
                                attrib.texcoords.at(2 * tex_index_b + 1))
                    : glm::vec2(0.0f),
                has_materials
                    ? glm::vec2(attrib.texcoords.at(2 * tex_index_c),
                                attrib.texcoords.at(2 * tex_index_c + 1))
                    : glm::vec2(0.0f),
                has_materials ? shape.mesh.material_ids[i / 3] : -1);

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
                if (loaded_textures.contains(tri.face_mat_id)) {
                    auto texture = loaded_textures[tri.face_mat_id];
                    texture_pixels = std::get<0>(texture);
                    texture_width = std::get<1>(texture);
                    texture_height = std::get<2>(texture);
                } else {
                    const tinyobj::material_t &mat = materials[tri.face_mat_id];
                    const std::string &diffuse_texname = mat.diffuse_texname;
                    if (diffuse_texname.empty()) {
                        has_materials = false;
                    } else {
                        int channels;
                        texture_pixels =
                            stbi_load((directory + diffuse_texname).c_str(),
                                      &texture_width, &texture_height,
                                      &channels, STBI_rgb_alpha);
                        if (!texture_pixels) {
                            std::stringstream ss;
                            ss << "Couldn't load texture from the following "
                                  "path: ";
                            ss << (directory + diffuse_texname);
                            ss << "\n";
                            ASSERT(false, ss.str());
                        }
                        loaded_textures[tri.face_mat_id] = {
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
                        if (tri.tri_aabb(
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

bool Triangle::tri_aabb_sat(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 aabb,
                  glm::vec3 axis) const {
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

bool Triangle::tri_aabb(glm::vec3 aabb_center, glm::vec3 aabb_extents) const {
    glm::vec3 x = a - aabb_center;
    glm::vec3 y = b - aabb_center;
    glm::vec3 z = c - aabb_center;

    glm::vec3 ab = glm::normalize(y - x);
    glm::vec3 bc = glm::normalize(z - y);
    glm::vec3 ca = glm::normalize(x - z);

    glm::vec3 a00 = glm::vec3(0.0, -ab.z, ab.y);
    glm::vec3 a01 = glm::vec3(0.0, -bc.z, bc.y);
    glm::vec3 a02 = glm::vec3(0.0, -ca.z, ca.y);

    glm::vec3 a10 = glm::vec3(ab.z, 0.0, -ab.x);
    glm::vec3 a11 = glm::vec3(bc.z, 0.0, -bc.x);
    glm::vec3 a12 = glm::vec3(ca.z, 0.0, -ca.x);

    glm::vec3 a20 = glm::vec3(-ab.y, ab.x, 0.0);
    glm::vec3 a21 = glm::vec3(-bc.y, bc.x, 0.0);
    glm::vec3 a22 = glm::vec3(-ca.y, ca.x, 0.0);

    if (!tri_aabb_sat(x, y, z,
                      aabb_extents, a00) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a01) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a02) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a10) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a11) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a12) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a20) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a21) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, a22) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, glm::vec3(1, 0, 0)) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, glm::vec3(0, 1, 0)) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, glm::vec3(0, 0, 1)) ||
        !tri_aabb_sat(x, y, z,
                      aabb_extents, glm::cross(ab, bc))) {
        return false;
    }

    return true;
}

Model::Model(std::string_view filepath) {
    std::cout << "Loading model: " << filepath << std::endl;

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

    float max = std::numeric_limits<float>::max();
    float min = std::numeric_limits<float>::min();
    min_ = glm::vec3(max, max, max);
    max_ = glm::vec3(min, min, min);

    for (uint64_t i = 0; i < attrib.vertices.size(); i += 3) {
        min_.x = fmin(min_.x, attrib.vertices[i]);
        min_.y = fmin(min_.y, attrib.vertices[i + 1]);
        min_.z = fmin(min_.z, attrib.vertices[i + 2]);
        max_.x = fmax(max_.x, attrib.vertices[i]);
        max_.y = fmax(max_.y, attrib.vertices[i + 1]);
        max_.z = fmax(max_.z, attrib.vertices[i + 2]);
    }
    std::cout << "Bottom corner: (" 
              << min_.x << ", " << min_.y << ", " << min_.z
              << ")\n";
    std::cout << "Top corner: (" 
              << max_.x << ", " << max_.y << ", " << max_.z
              << ")\n";

    for (const auto &shape : shapes) {
        std::cout << shape.mesh.indices.size() / 3 << " triangles\n";
        for (uint64_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            const auto &vert_a = shape.mesh.indices[i];
            const auto &vert_b = shape.mesh.indices[i + 1];
            const auto &vert_c = shape.mesh.indices[i + 2];

            auto index_a = vert_a.vertex_index;
            auto index_b = vert_b.vertex_index;
            auto index_c = vert_c.vertex_index;

            auto tex_index_a = vert_a.texcoord_index;
            auto tex_index_b = vert_b.texcoord_index;
            auto tex_index_c = vert_c.texcoord_index;

            has_materials_ =
                (tex_index_a >= 0 && tex_index_b >= 0 && tex_index_c >= 0);

            bool has_texture = has_materials_;
            int texture_width, texture_height;
            stbi_uc *texture_pixels;
            if (has_materials_) {
                int face_mat_id = shape.mesh.material_ids[i / 3];
                if (loaded_textures_.contains(face_mat_id)) {
                    auto texture = loaded_textures_[face_mat_id];
                    texture_pixels = std::get<0>(texture);
                    texture_width = std::get<1>(texture);
                    texture_height = std::get<2>(texture);
                } else {
                    const tinyobj::material_t &mat = materials[face_mat_id];
                    const std::string &diffuse_texname = mat.diffuse_texname;
                    if (diffuse_texname.empty()) {
                        has_texture = false;
                    } else {
                        int channels;
                        texture_pixels =
                            stbi_load((directory + diffuse_texname).c_str(),
                                      &texture_width, &texture_height,
                                      &channels, STBI_rgb_alpha);
                        if (!texture_pixels) {
                            std::stringstream ss;
                            ss << "Couldn't load texture from the following "
                                  "path: ";
                            ss << (directory + diffuse_texname);
                            ss << "\n";
                            ASSERT(false, ss.str());
                        }
                        loaded_textures_[face_mat_id] = {
                            texture_pixels, texture_width, texture_height};
                    }
                }
            }

            triangles_.emplace_back(
                glm::vec3(attrib.vertices.at(3 * index_a),
                          attrib.vertices.at(3 * index_a + 1),
                          attrib.vertices.at(3 * index_a + 2)),
                glm::vec3(attrib.vertices.at(3 * index_b),
                          attrib.vertices.at(3 * index_b + 1),
                          attrib.vertices.at(3 * index_b + 2)),
                glm::vec3(attrib.vertices.at(3 * index_c),
                          attrib.vertices.at(3 * index_c + 1),
                          attrib.vertices.at(3 * index_c + 2)),
                has_materials_
                    ? glm::vec2(attrib.texcoords.at(2 * tex_index_a),
                                attrib.texcoords.at(2 * tex_index_a + 1))
                    : glm::vec2(0.0f),
                has_materials_
                    ? glm::vec2(attrib.texcoords.at(2 * tex_index_b),
                                attrib.texcoords.at(2 * tex_index_b + 1))
                    : glm::vec2(0.0f),
                has_materials_
                    ? glm::vec2(attrib.texcoords.at(2 * tex_index_c),
                                attrib.texcoords.at(2 * tex_index_c + 1))
                    : glm::vec2(0.0f),
                has_materials_ ? shape.mesh.material_ids[i / 3] : -1,
                has_texture);
        }
    }
}

Model::~Model() {
    for (auto [_, tex] : loaded_textures_) {
        stbi_image_free(std::get<0>(tex));
    }
}

Voxelizer::Voxelizer(std::string_view filepath,
                     std::tuple<uint32_t, uint32_t, uint32_t> total_size)
    : 
    model_{filepath}
{
    glm::vec3 max = model_.get_max();
    glm::vec3 min = model_.get_min();
    width_ = std::get<0>(total_size);
    height_ = std::get<1>(total_size);
    depth_ = std::get<2>(total_size);
    std::cout << "Chunk Size: (" 
              << width_ << ", " << height_ << ", " << depth_ 
              << ")\n";

    voxel_size_ = std::numeric_limits<float>::min();
    voxel_size_ = fmax(float(max.x - min.x) / width_, voxel_size_);
    voxel_size_ = fmax(float(max.y - min.y) / height_, voxel_size_);
    voxel_size_ = fmax(float(max.z - min.z) / depth_, voxel_size_);

    std::cout << "Voxel Size: " << voxel_size_ << std::endl;

    // Allocate the number of voxel chunks that we'll need.
    chunks_width_ = static_cast<uint32_t>(ceil((float)width_ / voxel_chunk_size_));
    chunks_height_ = static_cast<uint32_t>(ceil((float)height_ / voxel_chunk_size_));
    chunks_depth_ = static_cast<uint32_t>(ceil((float)depth_ / voxel_chunk_size_));

    voxel_chunks_.resize(chunks_width_ * chunks_height_ * chunks_depth_);

    std::cout << "Voxelizer Size: (" 
              << chunks_width_ << ", " << chunks_height_ << ", " << chunks_depth_ 
              << ")\n";

    std::filesystem::path filename = std::filesystem::path(filepath).filename();
    std::stringstream string_pointer;
    string_pointer << filename.string() << "_" << width_ << "_" << height_
                   << "_" << depth_ << "_" << voxel_chunk_size_;
    voxels_directory_ = "disk_voxels_" + string_pointer.str();
    std::filesystem::create_directory(voxels_directory_);
}

Voxelizer::~Voxelizer() {
    // std::filesystem::remove_all(voxels_directory_);
    for (uint32_t chunk_x = 0; chunk_x < chunks_width_; chunk_x++) {
        for (uint32_t chunk_y = 0; chunk_y < chunks_height_; chunk_y++) {
            for (uint32_t chunk_z = 0; chunk_z < chunks_width_; chunk_z++) {
                uint32_t voxel_chunk_index =
                    linearize_chunk_index(chunk_x, chunk_y, chunk_z);
                VoxelizedChunk &voxel_chunk =
                    voxel_chunks_.at(voxel_chunk_index);
                
                std::filesystem::path chunk_path =
                    voxels_directory_ / std::to_string(voxel_chunk_index);

                if (std::filesystem::exists(chunk_path)) {
                    continue;
                }

                if (!voxel_chunk.empty()) {
                    write_voxels_to_disk(voxel_chunk_index);
                }
            }
        }
    }
}

inline uint32_t Voxelizer::linearize_chunk_index(uint32_t x, uint32_t y, uint32_t z) const {
    return x + chunks_width_ * y + chunks_width_ * chunks_height_ * z;
}

void Voxelizer::voxelize_chunk(uint32_t chunk_x, uint32_t chunk_y, uint32_t chunk_z) {
    std::cout << "INFO: Voxelizing Chunk (" << chunk_x << ", " << chunk_y << ", " << chunk_z << ")\n";
    // If this function is being called, we can assume that the chunk isn't already in memory.
    // We first check if the chunk instead exists on the disk.
    uint32_t voxel_chunk_index =
        linearize_chunk_index(chunk_x, chunk_y, chunk_z);
    VoxelizedChunk &voxel_chunk = voxel_chunks_.at(voxel_chunk_index);
    ASSERT(voxel_chunk.empty(), "Only voxelize the chunk if it's empty!");
    std::filesystem::path chunk_path =
        voxels_directory_ / std::to_string(voxel_chunk_index);

    uint64_t added_memory =
        voxel_chunk_size_ * voxel_chunk_size_ * voxel_chunk_size_ * 4;
     
    // If the chunk doesn't exist on the disk, then we have to voxelize the chunk from scratch.
    // We should first check that we aren't exceeding the maximum memory size,
    // otherwise we should first write another less-used chunk to the disk.
    
    // todo: right now, this is just going to write the lowest voxel index out to disk.
    // A better solution would be to maintain some LRU of some sort.
    while (current_memory_usage_ + added_memory > max_memory_usage_) {
        // Note: each voxel chunk should take the same amount of memory,
        // so a while loop here shouldn't be necessary (need to double check)
        std::optional<uint32_t> old_chunk = find_old_voxel_chunk(voxel_chunk_index);
        if (old_chunk.has_value()) {
            write_voxels_to_disk(old_chunk.value());
        } else {
            ASSERT(false, "Out of available memory but there are no old chunks to write?!");
        }
    }

    if (std::filesystem::exists(chunk_path)) {
        read_voxels_from_disk(voxel_chunk_index);
        return;
    }

    // At this point we have enough available memory to voxelize the chunk
    uint32_t min_chunk_x = voxel_chunk_size_ * chunk_x;
    uint32_t min_chunk_y = voxel_chunk_size_ * chunk_y;
    uint32_t min_chunk_z = voxel_chunk_size_ * chunk_z;
    uint32_t max_chunk_x = min_chunk_x + voxel_chunk_size_;
    uint32_t max_chunk_y = min_chunk_y + voxel_chunk_size_;
    uint32_t max_chunk_z = min_chunk_z + voxel_chunk_size_;

    const std::vector<Triangle>& triangles = model_.get_triangles();
    bool has_materials = model_.has_materials();
    const auto& textures = model_.get_textures();
    glm::vec3 min = model_.get_min();
    glm::vec3 max = model_.get_max();

    voxel_chunk.resize(static_cast<size_t>(voxel_chunk_size_) *
                       static_cast<size_t>(voxel_chunk_size_) *
                       static_cast<size_t>(voxel_chunk_size_) * 4,
                       static_cast<std::byte>(0));
    
    for (const auto &tri : triangles) {
        uint32_t min_voxel_x = static_cast<uint32_t>(
            floor((tri.min_x() - min.x) / voxel_size_));
        uint32_t min_voxel_y = static_cast<uint32_t>(
            floor((tri.min_y() - min.y) / voxel_size_));
        uint32_t min_voxel_z = static_cast<uint32_t>(
            floor((tri.min_z() - min.z) / voxel_size_));
        uint32_t max_voxel_x = static_cast<uint32_t>(
            ceil((tri.max_x() - min.x) / voxel_size_));
        uint32_t max_voxel_y = static_cast<uint32_t>(
            ceil((tri.max_y() - min.y) / voxel_size_));
        uint32_t max_voxel_z = static_cast<uint32_t>(
            ceil((tri.max_z() - min.z) / voxel_size_));

        // If the voxel lies outside of the chunk, we can skip trying to voxelize it
        if (min_voxel_x > max_chunk_x || min_voxel_y > max_chunk_y || min_voxel_z > max_chunk_z || 
            max_voxel_x < min_chunk_x || max_voxel_y < min_chunk_y || max_voxel_z < min_chunk_z) {
            continue;
        }

        min_voxel_x = std::max(min_voxel_x, min_chunk_x);
        min_voxel_y = std::max(min_voxel_y, min_chunk_y);
        min_voxel_z = std::max(min_voxel_z, min_chunk_z);
        max_voxel_x = std::min(max_voxel_x, max_chunk_x - 1);
        max_voxel_y = std::min(max_voxel_y, max_chunk_y - 1);
        max_voxel_z = std::min(max_voxel_z, max_chunk_z - 1);

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
            if (textures.contains(tri.face_mat_id)) {
                const auto texture = textures.at(tri.face_mat_id);
                texture_pixels = std::get<0>(texture);
                texture_width = std::get<1>(texture);
                texture_height = std::get<2>(texture);
            }
        }

        for (uint32_t x = min_voxel_x; x <= max_voxel_x; ++x) {
            for (uint32_t y = min_voxel_y; y <= max_voxel_y; ++y) {
                for (uint32_t z = min_voxel_z; z <= max_voxel_z; ++z) {
                    float tri_voxel_x =
                        static_cast<float>(x) * voxel_size_ + min.x;
                    float tri_voxel_y =
                        static_cast<float>(y) * voxel_size_ + min.y;
                    float tri_voxel_z =
                        static_cast<float>(z) * voxel_size_ + min.z;
                    if (tri.tri_aabb(
                            glm::vec3(tri_voxel_x, tri_voxel_y, tri_voxel_z),
                            glm::vec3(voxel_size_))) {
                        std::byte r = static_cast<std::byte>(255),
                                  g = static_cast<std::byte>(255),
                                  b = static_cast<std::byte>(255),
                                  a = static_cast<std::byte>(255);
                        glm::vec3 plane_point =
                            glm::vec3(tri_voxel_x, tri_voxel_y, tri_voxel_z);
                        plane_point -=
                            unit_plane *
                            (glm::dot(unit_plane, plane_point - tri.a));
                        glm::vec3 barycentric_coords =
                            inverse_tri_matrix * plane_point;

                        if (tri.has_texture) {
                            glm::vec2 uv = tri.t_a * barycentric_coords.x +
                                           tri.t_b * barycentric_coords.y +
                                           tri.t_c * barycentric_coords.z;
                            int tex_x = static_cast<int>(uv.x * texture_width);
                            int tex_y = static_cast<int>(uv.y * texture_height);
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
                            int linear_coord = tex_x + tex_y * texture_width;

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
                            (x - min_chunk_x) + (y - min_chunk_y) * voxel_chunk_size_ +
                            (z - min_chunk_z) * voxel_chunk_size_ * voxel_chunk_size_;
                        voxel_chunk.at(voxel_idx * 4) = r;
                        voxel_chunk.at(voxel_idx * 4 + 1) = g;
                        voxel_chunk.at(voxel_idx * 4 + 2) = b;
                        voxel_chunk.at(voxel_idx * 4 + 3) = a;
                    }
                }
            }
        }
    }

    current_memory_usage_ += added_memory;
    std::cout << "INFO: After voxelizing, " << added_memory << " bytes were allocated, so the total is now " << current_memory_usage_ << " bytes.\n";
}

std::optional<uint32_t> Voxelizer::find_old_voxel_chunk(uint32_t chunk_to_voxelize) const {
    for (uint32_t x = 0; x < chunks_width_; x++) {
        for (uint32_t y = 0; y < chunks_height_; y++) {
            for (uint32_t z = 0; z < chunks_width_; z++) {
                uint32_t chunk_index = linearize_chunk_index(x, y, z);
                if (!voxel_chunks_.at(chunk_index).empty() && (chunk_index != chunk_to_voxelize)) {
                    return std::optional<uint32_t>(chunk_index);
                }
            }
        }
    }

    return std::nullopt;
}

uint32_t Voxelizer::at(uint32_t x, uint32_t y, uint32_t z) {
    static_assert(4 * sizeof(std::byte) == sizeof(uint32_t));
    if (x >= width_ || y >= height_ || z >= depth_) {
        return 0;
    }
    
    y = height_ - y - 1;
    
    uint32_t chunk_x = x / voxel_chunk_size_;
    uint32_t chunk_y = y / voxel_chunk_size_;
    uint32_t chunk_z = z / voxel_chunk_size_;
    uint32_t in_chunk_x = x % voxel_chunk_size_;
    uint32_t in_chunk_y = y % voxel_chunk_size_;
    uint32_t in_chunk_z = z % voxel_chunk_size_;
    uint32_t in_chunk_index = in_chunk_x + voxel_chunk_size_ * in_chunk_y + voxel_chunk_size_ * voxel_chunk_size_ * in_chunk_z;
    VoxelizedChunk *voxel_chunk = last_chunk_;
    if (chunk_x != last_chunk_x_ || chunk_x != last_chunk_x_ || chunk_x != last_chunk_x_) {
	uint32_t chunk_index = linearize_chunk_index(chunk_x, chunk_y, chunk_z);
	voxel_chunk = &voxel_chunks_.at(chunk_index);
	if (voxel_chunk->empty()) {
	    voxelize_chunk(chunk_x, chunk_y, chunk_z);
	}
	last_chunk_x_ = chunk_x;
	last_chunk_y_ = chunk_y;
	last_chunk_z_ = chunk_z;
	last_chunk_ = voxel_chunk;
    }

    uint32_t voxel;
    memcpy(&voxel, voxel_chunk->data() + 4 * in_chunk_index, sizeof(uint32_t));
    return voxel;
}

void Voxelizer::write_voxels_to_disk(uint32_t chunk_index) {
    VoxelizedChunk &voxel_chunk = voxel_chunks_.at(chunk_index);
    std::cout << "INfO: Writing chunk at index " << chunk_index << " to disk.\n";
    if (actually_write_to_disk_) {
	
	std::filesystem::path chunk_path =
	    voxels_directory_ /
	    std::to_string(chunk_index);
	
	if (!std::filesystem::exists(chunk_path)) {
	    std::ofstream stream(chunk_path, std::ios::out | std::ios::binary);
	    stream.write(reinterpret_cast<char *>(voxel_chunk.data()),
			 voxel_chunk.size());
	}
    }
    
    current_memory_usage_ -= voxel_chunk.size();
    voxel_chunks_.at(chunk_index).clear();
    voxel_chunks_.at(chunk_index).shrink_to_fit();
    ASSERT(voxel_chunks_.at(chunk_index).size() == 0, "Voxel chunk should be empty after being written to disk!");

    uint64_t actual_memory_usage = 0;
    for (const auto &chunk : voxel_chunks_) {
	actual_memory_usage += chunk.capacity();
    }
    std::cout << "INFO: After writing, the amount of memory used for raw voxel chunks is " << actual_memory_usage << " bytes.\n";
}

void Voxelizer::read_voxels_from_disk(uint32_t chunk_index) {
    VoxelizedChunk &voxel_chunk = voxel_chunks_.at(chunk_index);

    std::filesystem::path chunk_path =
        voxels_directory_ / std::to_string(chunk_index);
    std::ifstream stream(chunk_path, std::ios::in | std::ios::binary);
    const auto size = std::filesystem::file_size(chunk_path);
    voxel_chunk.resize(size);
    stream.read(reinterpret_cast<char *>(voxel_chunk.data()), size);

    current_memory_usage_ += voxel_chunk.size();
}
