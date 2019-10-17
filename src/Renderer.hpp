#pragma once
#include <string>
#include <vector>

#include <CL/cl.hpp>

#include "float3.h"
#include "Scene.hpp"

class Renderer{
private:
    void get_platform();
    void get_device();
    void create_from_file_and_build(std::string kernel_filename);
    void bloom();
    std::vector<float3> output;
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    cl::CommandQueue queue;
    const int width;
    const int height;
    const int samples;
    const int bloom_rad;
public:
    Renderer(std::string kernel_filename, int width, int height, int samples, int radius);
    void render(Scene& scene);
    void save_image(std::string filename);
};
