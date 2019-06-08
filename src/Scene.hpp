#pragma once

#include <map>
#include <string>
#include <vector>

#include "Camera.h"
#include "Material.h"
#include "Triangle.h"

class Scene{
private:
    std::map<std::string, int> material_idx;
    void load_materials(std::string filename);
    void load_camera(std::string filename);
public:
    std::vector<Triangle> triangles;
    std::vector<Material> materials;
    Camera camera;
    Scene(std::string filename);
};
