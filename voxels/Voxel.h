#include <vector>
#include "external/Vector/vector.h"
#include <map>
#include <string>

class Voxel {
    public:
    Voxel(float x, float y, float z);
    Voxel(vector3D<float> pos);
    ~Voxel();

    void AddProperty();
    private:
    vector3D<float> position_; // need to decide float vs double for memory vs accuracy
    //TODO: map for properties or other storage form


};