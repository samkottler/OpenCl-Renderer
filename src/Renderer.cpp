#include <fstream>
#include <streambuf>
#include <string>
#include <vector>

#include <CL/cl.hpp>

#include "error.hpp"
#include "lodepng.h"
#include "Renderer.hpp"
#include "Scene.hpp"

void Renderer::get_platform(){
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
        print_error("No platform found. Check OpenCL installation.");
    platform = all_platforms[0];

    std::clog << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
}

void Renderer::get_device(){
    std::vector<cl::Device> all_devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if (all_devices.size() == 0){
	platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if (all_devices.size() == 0)
	    print_error("NO devices found. Check OpenCL installation.");
	else
	    print_warning("Using non-GPU device. GPU recommended.");
    }
    device = all_devices[0];

    std::clog << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
}

void Renderer::create_from_file_and_build(std::string kernel_filename){
    std::ifstream stream(kernel_filename);
    std::string source((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    cl::Program::Sources sources;
    sources.push_back({source.c_str(), source.length()});
    program = cl::Program(context, sources);
    if (program.build({device}, "-I src") != CL_SUCCESS){
	print_error(program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
    }
    else
	std::clog << "Sucessfully built program." << std::endl;
    queue = cl::CommandQueue(context, device);
}

Renderer::Renderer(std::string kernel_filename, int w, int h) : width(w), height(h){
    get_platform();
    get_device();
    context = cl::Context({device});
    create_from_file_and_build(kernel_filename);
    image = std::vector<unsigned char>(width*height*4);
}

void Renderer::render(Scene scene){
    
    cl::Buffer out_buf(context, CL_MEM_READ_WRITE, sizeof(cl_float3)*width*height);

    cl::Kernel kernel = cl::Kernel(program, "render");    
    cl::make_kernel<cl::Buffer> render_kernel(kernel);
    cl::EnqueueArgs eargs(queue, cl::NullRange, cl::NDRange(width,height), cl::NDRange(8,8));
    render_kernel(eargs, out_buf).wait();

    std::vector<cl_float3> output(width*height);
    queue.enqueueReadBuffer(out_buf, CL_TRUE, 0, sizeof(cl_float3)*width*height, output.data());
    
    for (int i = 0; i < width*height; ++i){
	image[4*i + 0] = output[i].x*255;
	image[4*i + 1] = output[i].y*255;
	image[4*i + 2] = output[i].z*255;
	image[4*i + 3] = 255;
    }
}

void Renderer::save_image(std::string filename){
    lodepng::encode(filename, image, width, height);
}


