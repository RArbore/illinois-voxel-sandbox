const vec3 voxel_normals[6] = vec3[6](
				      vec3(-1.0, 0.0, 0.0), // normal for -x
				      vec3(1.0, 0.0, 0.0),  // normal for x
				      vec3(0.0, -1.0, 0.0), // normal for -y
				      vec3(0.0, 1.0, 0.0),  // normal for y
				      vec3(0.0, 0.0, -1.0), // normal for -z
				      vec3(0.0, 0.0, 1.0)   // normal for z
				      );

const vec3 voxel_tangents[6] = vec3[6](
                      vec3(0.0, 1.0, 0.0), // tangent for -x
                      vec3(0.0, 1.0, 0.0), // tangent for x
                      vec3(1.0, 0.0, 0.0), // tangent for -y
                      vec3(1.0, 0.0, 0.0), // tangent for y
                      vec3(0.0, 1.0, 0.0), // tangent for -z
                      vec3(0.0, 1.0, 0.0) // tangent for z
                      );

const vec3 voxel_bitangents[6] = vec3[6](
                      vec3(0.0, 0.0, -1.0),  // bitangent for -x
                      vec3(0.0, 0.0, 1.0), // bitangent for x
                      vec3(0.0, 0.0, 1.0),  // bitangent for -y
                      vec3(0.0, 0.0, -1.0), // bitangent for y
                      vec3(1.0, 0.0, 0.0),  // bitangent for -z
                      vec3(-1.0, 0.0, 0.0)  // bitangent for z
                      );

// Need to transform between local direction sampled from hemisphere and world normal
vec3 local_to_world(const vec3 wo, uint voxel_face) {
    vec3 normal = voxel_normals[voxel_face];
    vec3 tangent = voxel_tangents[voxel_face];
    vec3 bitangent = voxel_bitangents[voxel_face];

    return vec3(
        tangent.x * wo.x + normal.x * wo.y + bitangent.x * wo.z,
        tangent.y * wo.x + normal.y * wo.y + bitangent.y * wo.z,
        tangent.y * wo.z + normal.z * wo.z + bitangent.z * wo.z
    );
}

// Want to evaluate incoming light direction in local space
vec3 world_to_local(const vec3 wi, uint voxel_face) {
    vec3 normal = voxel_normals[voxel_face];
    vec3 tangent = voxel_tangents[voxel_face];
    vec3 bitangent = voxel_bitangents[voxel_face];

    return vec3(dot(wi, tangent), dot(wi, normal), dot(wi, bitangent));
}