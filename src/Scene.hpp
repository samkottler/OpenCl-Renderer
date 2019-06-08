#pragma once

#include <map>
#include <string>
#include <vector>

#include "BVH.hpp"
#include "Camera.h"
#include "Material.h"
#include "Triangle.h"

class Scene{
private:
    std::map<std::string, int> material_idx;
    std::vector<Triangle> triangles;
    void load_materials(std::string filename);
    void load_camera(std::string filename);
    void load_ply(std::string filename, int mat_idx, float3 translate, float scale, float3 xaxis, float3 yaxis);
public:
    BVH bvh;
    std::vector<Material> materials;
    Camera camera;
    Scene(std::string filename);
};
