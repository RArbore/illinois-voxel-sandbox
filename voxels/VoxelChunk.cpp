#include "VoxelChunk.h"


enum Type { basic = 0, Int = 1, Float = 2, Vec3 = 3 };

VoxelChunk CreateChunk() {
    glm::vec3 position_ = glm::vec3(0, 0, 0);
    std::shared_ptr chunk_(malloc(CalculateSize(0, 0, 0, basic)), free);
    *(chunk_) = 0;
    *(chunk_+4) = 0;
    *(chunk_+8) = 0;
    *(chunk_+12) = 0;
    *(chunk_+13) = 0;
    *(chunk_+14) = 0;

}

VoxelChunk CreateChunk(float x, float y, float z, int w, int d, int h) {
    glm::vec3 position_ = glm::vec3(x, y, z);
    std::shared_ptr chunk_(malloc(CalculateSize(w, d, h, basic)), free);
    //probably doing this wrong, lmk if there is a better way to write the bytes directly
    *(chunk_) = x;
    *(chunk_+4) = y;
    *(chunk_+8) = z;
    *(chunk_+12) = w;
    *(chunk_+13) = d;
    *(chunk_+14) = h;
    for (int i = 0; i < std::ceil((w * d * h) / 8)) {
        *(chunk_+15+i) = 0;
    }
}

uint16_t CalculateSize(int w, int d, int h, Type t) {
    /** This will depend on exactly how we lay out the voxel data
     * 
     **/ 
    uint16_t size;
    switch (t) {
        case basic:
            // 3 floats for position, 3 bytes for dimensions, w * d * h / 8 for bitmap
            size = sizeof(float) * 3 + 3 + std::ceil((w * d * h) / 8);
            break;
    }
    return size;
}