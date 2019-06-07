#pragma once
#include <string>
#include <vector>

#include <CL/cl.hpp>

#include "Scene.hpp"

class Renderer{
private:
    void get_platform();
    void get_device();
    void create_from_file_and_build(std::string kernel_filename);
    std::vector<unsigned char> image;
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    cl::CommandQueue queue;
    const int width;
    const int height;
    const int samples;
public:
    Renderer(std::string kernel_filename, int width, int height, int samples);
    void render(Scene scene);
    void save_image(std::string filename);
};
