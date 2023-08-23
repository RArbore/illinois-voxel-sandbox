#include "Voxel.h"

Voxel::Voxel(float x, float y, float z) {
    position_ = vector3D<float>(x, y, z);
}

Voxel::Voxel(vector3D<float> pos) {
    position_ = Vector3d<float>(pos);
}

Voxel::~Voxel() {
    
}

