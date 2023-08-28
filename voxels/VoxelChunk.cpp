#include "VoxelChunk.h"

VoxelChunk::VoxelChunk(VoxelFormat format, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f)) :
    format_{format},
    position_{position}
{}

size_t VoxelChunk::calculate_size(int w, int d, int h) {
    /** This will depend on exactly how we lay out the voxel data
     * 
     **/ 
    uint16_t size;
    switch (format_) {
    case VoxelFormat::Raw:
            // 3 floats for position, 3 bytes for dimensions, w * d * h / 8 for bitmap
            size = sizeof(float) * 3 + 3 + std::ceil((w * d * h) / 8);
            break;
    }
    return size;
}