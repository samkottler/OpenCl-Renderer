#pragma once
#include <string>
#include <vector>

#include "Scene.hpp"

class Renderer{
private:
    std::vector<unsigned char> image;
public:
    Renderer(std::string kernel_filename);
    void render(Scene scene);
    void save_image(std::string filename);
};
