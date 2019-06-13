#include <fstream>
#include <iostream>
#include <random>
#include <streambuf>
#include <string>
#include <vector>

#include <CL/cl.hpp>

#include "Camera.h"
#include "error.hpp"
#include "float3.h"
#include "lodepng.h"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.h"

inline float clamp(float x){return x<0.0? 0.0: x>1.0 ? 1.0 : x;}
inline int to_int(float x){return int(pow(clamp(x), 1/2.2)*255 + 0.5);}

void Renderer::get_platform(){
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size() == 0)
        print_error("No platform found. Check OpenCL installation.");
    platform = all_platforms[0];

    std::clog << "  Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
}

void Renderer::get_device(){
    std::vector<cl::Device> all_devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if (all_devices.size() == 0){
	platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if (all_devices.size() == 0)
	    print_error("No devices found. Check OpenCL installation.");
	else
	    print_warning("Using non-GPU device. GPU recommended.");
    }
    device = all_devices[0];

    std::clog << "  Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
}

void Renderer::create_from_file_and_build(std::string kernel_filename){
    std::ifstream stream(kernel_filename);
    std::string source((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    cl::Program::Sources sources;
    sources.push_back({source.c_str(), source.length()});
    program = cl::Program(context, sources);
    std::string flags = "-I src -DSAMPLES=" + std::to_string(samples);
    if (program.build({device}, flags.c_str()) != CL_SUCCESS){
	print_error(program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
    }
    else
	std::clog << "  Sucessfully built program." << std::endl;
    queue = cl::CommandQueue(context, device);
}

Renderer::Renderer(std::string kernel_filename, int w, int h, int s, int r) : width(w), height(h), samples(s), bloom_rad(r){
    std::clog << "Initializing OpenCL..." << std::endl;
    get_platform();
    get_device();
    context = cl::Context({device});
    create_from_file_and_build(kernel_filename);
}

void Renderer::bloom(){
    float stddev = 5.0/512*width;
    float kernel[(2*bloom_rad+1)*(2*bloom_rad+1)];
    float denom = 2*stddev*stddev;
    for (int y = -bloom_rad; y<=bloom_rad; y++){
	for (int x = -bloom_rad; x<=bloom_rad; x++){
	    kernel[(y+bloom_rad)*(2*bloom_rad+1)+x+bloom_rad] = exp(-(x*x+y*y)/denom)/denom/M_PI;
	}
    }
    std::vector<float3> temp(width*height);
    for (int i = 0; i<width*height; i++){
	float r,g,b;
	r=g=b=0;
	if (output[i].x > 1) r = output[i].x - 1;
	if (output[i].y > 1) g = output[i].y - 1;
	if (output[i].z > 1) b = output[i].z - 1;
        temp[i] = {r,g,b};
    }
    for (int y = 0; y<height; y++){
	for(int x = 0; x<width; x++){
	    float3 c = {0,0,0};
	    for (int ky = -bloom_rad; ky<=bloom_rad; ky++){
		for (int kx = -bloom_rad; kx<=bloom_rad; kx++){
		    if ((y+ky)>=0 && (y+ky)<height &&
			(x+kx)>=0 && (x+kx)<width){
			c = c+kernel[(ky+bloom_rad)*(2*bloom_rad+1)+kx+bloom_rad]*temp[(y+ky)*width+(x+kx)];
		    }
		}
	    }
	    output[y*width+x] = (1.0f/20)*c + output[y*width + x];
	}
    }
}

void Renderer::render(Scene scene){
    output = std::vector<float3>(width*height);
    std::default_random_engine rand_gen;
    for (int i = 0; i< output.size(); ++i){
	union {
	    int val;
	    float f;
	} u;
	u.val = rand_gen();
	output[i].x = u.f;
	u.val = rand_gen();
	output[i].y = u.f;
    }
    
    
    cl::Buffer out_buf(context, CL_MEM_READ_WRITE, sizeof(float3)*width*height);
    cl::Buffer bvh_buf(context, CL_MEM_READ_WRITE, sizeof(GPU_BVHnode)*scene.bvh.GPU_BVH.size());
    cl::Buffer triangle_buf(context, CL_MEM_READ_WRITE, sizeof(Triangle)*scene.bvh.ordered.size());
    cl::Buffer material_buf(context, CL_MEM_READ_WRITE, sizeof(Material)*scene.materials.size());

    queue.enqueueWriteBuffer(out_buf, CL_TRUE, 0, output.size()*sizeof(float3), output.data());
    queue.enqueueWriteBuffer(bvh_buf, CL_TRUE, 0, sizeof(GPU_BVHnode)*scene.bvh.GPU_BVH.size(),
			     scene.bvh.GPU_BVH.data());
    queue.enqueueWriteBuffer(triangle_buf, CL_TRUE, 0, sizeof(Triangle)*scene.bvh.ordered.size(),
			     scene.bvh.ordered.data());
    queue.enqueueWriteBuffer(material_buf, CL_TRUE, 0, sizeof(Material)*scene.materials.size(),
			     scene.materials.data());
    
    cl::Kernel kernel = cl::Kernel(program, "render");    
    cl::make_kernel<cl::Buffer, cl::Buffer, cl::Buffer, cl::Buffer, Camera> render_kernel(kernel);
    cl::EnqueueArgs eargs(queue, cl::NullRange, cl::NDRange(width,height), cl::NDRange(8,8));

    std::clog << "Starting render..." << std::endl;
    render_kernel(eargs, out_buf, bvh_buf, triangle_buf, material_buf, scene.camera).wait();

    queue.enqueueReadBuffer(out_buf, CL_TRUE, 0, sizeof(float3)*width*height, output.data());
}

void Renderer::save_image(std::string filename){
    std::clog << "Saving image ..." << std::endl;

    bloom();

    std::vector<unsigned char> image = std::vector<unsigned char>(width*height*4);
    for (int i = 0; i < width*height; ++i){
	image[4*i + 0] = to_int(output[i].x);
	image[4*i + 1] = to_int(output[i].y);
	image[4*i + 2] = to_int(output[i].z);
	image[4*i + 3] = 255;
    }
    
    lodepng::encode(filename, image, width, height);
}


