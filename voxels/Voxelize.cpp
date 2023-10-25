#include <filesystem>

#include <external/glm/glm/glm.hpp>
#include <external/tinyobjloader/tiny_obj_loader.h>

#include "Voxelize.h"
#include "Conversion.h"
#include "utils/Assert.h"

struct Triangle {
    glm::vec3 a, b, c;

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
    ASSERT(reader.Warning().empty(), reader.Warning());
    
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    
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
    std::cout << shapes[0].mesh.indices.size() / 3 << " triangles\n";

    uint32_t chunk_width = static_cast<uint32_t>(ceil((max.x - min.x) / voxel_size));
    uint32_t chunk_height = static_cast<uint32_t>(ceil((max.y - min.y) / voxel_size));
    uint32_t chunk_depth = static_cast<uint32_t>(ceil((max.z - min.z) / voxel_size));
    std::cout << "Chunk size: (" << chunk_width << ", " << chunk_height << ", " << chunk_depth << ")\n";

    std::vector<Triangle> triangles;
    for (int64_t i = 0; i < shapes[0].mesh.indices.size(); i += 3) {
	auto index_a = shapes[0].mesh.indices[i].vertex_index;
	auto index_b = shapes[0].mesh.indices[i + 1].vertex_index;
	auto index_c = shapes[0].mesh.indices[i + 2].vertex_index;
	triangles.emplace_back(
			       glm::vec3(attrib.vertices[3 * index_a], attrib.vertices[3 * index_a + 1], attrib.vertices[3 * index_a + 2]),
			       glm::vec3(attrib.vertices[3 * index_b], attrib.vertices[3 * index_b + 1], attrib.vertices[3 * index_b + 2]),
			       glm::vec3(attrib.vertices[3 * index_c], attrib.vertices[3 * index_c + 1], attrib.vertices[3 * index_c + 2])
			       );
    }

    std::vector<std::byte> data(static_cast<size_t>(chunk_width) *
                                    static_cast<size_t>(chunk_height) *
                                    static_cast<size_t>(chunk_depth) * 4,
                                static_cast<std::byte>(0));
    std::byte red = static_cast<std::byte>(255);
    std::byte green = static_cast<std::byte>(255);
    std::byte blue = static_cast<std::byte>(255);
    std::byte alpha = static_cast<std::byte>(255);
    for (const auto &tri : triangles) {
	uint32_t min_voxel_x = static_cast<uint32_t>(floor(((tri.min_x() - min.x) / (max.x - min.x)) * static_cast<float>(chunk_width)));
	uint32_t min_voxel_y = static_cast<uint32_t>(floor(((tri.min_y() - min.y) / (max.y - min.y)) * static_cast<float>(chunk_height)));
	uint32_t min_voxel_z = static_cast<uint32_t>(floor(((tri.min_z() - min.z) / (max.z - min.z)) * static_cast<float>(chunk_depth)));
	uint32_t max_voxel_x = static_cast<uint32_t>(ceil(((tri.max_x() - min.x) / (max.x - min.x)) * static_cast<float>(chunk_width)));
	uint32_t max_voxel_y = static_cast<uint32_t>(ceil(((tri.max_y() - min.y) / (max.y - min.y)) * static_cast<float>(chunk_height)));
	uint32_t max_voxel_z = static_cast<uint32_t>(ceil(((tri.max_z() - min.z) / (max.z - min.z)) * static_cast<float>(chunk_depth)));
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
		    float tri_voxel_x = static_cast<float>(x) / chunk_width * (max.x - min.x) + min.x;
		    float tri_voxel_y = static_cast<float>(y) / chunk_height * (max.y - min.y) + min.y;
		    float tri_voxel_z = static_cast<float>(z) / chunk_depth * (max.z - min.z) + min.z;
		    if (tri_aabb(tri, glm::vec3(tri_voxel_x + 0.5f, tri_voxel_y + 0.5f, tri_voxel_z + 0.5f), glm::vec3(1.0f))) {
			size_t voxel_idx = x + (chunk_height - y - 1) * chunk_width + z * chunk_width * chunk_height;
			data.at(voxel_idx * 4) = red;
			data.at(voxel_idx * 4 + 1) = green;
			data.at(voxel_idx * 4 + 2) = blue;
			data.at(voxel_idx * 4 + 3) = alpha;
		    }
		}
	    }
	}
    }

    out_chunk_width = chunk_width;
    out_chunk_height = chunk_height;
    out_chunk_depth = chunk_depth;
    return data;
}
