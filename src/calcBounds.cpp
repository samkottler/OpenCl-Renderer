#include <iostream>
#include <fstream>
#include <vector>

#include <CL/cl.hpp>

#include <cstdio>

#include "error.hpp"
#include "tiny_obj_loader.h"
#include "tinyply.h"
#include "float3.h"

namespace tp = tinyply;
namespace to = tinyobj;

int main(int argc, char** argv){
    std::string filename = argv[1];

    float s = std::stof(argv[2]);

    float3 xaxis = {std::stof(argv[3]), std::stof(argv[4]), std::stof(argv[5])};
    float3 yaxis = {std::stof(argv[6]), std::stof(argv[7]), std::stof(argv[8])};
    float3 zaxis = cross(xaxis, yaxis);

    float temp  = zaxis.x;
    zaxis.x = xaxis.z;
    xaxis.z = temp;
    temp = zaxis.y;
    zaxis.y = yaxis.z;
    yaxis.z = temp;
    temp = xaxis.y;
    xaxis.y = yaxis.x;
    yaxis.x = temp;

    float minx = 1000, miny = 1000, minz = 1000;
    float maxx = -1000, maxy = -1000, maxz = -1000;

    float3 t = {std::stof(argv[9]), std::stof(argv[10]), std::stof(argv[11])};

    std::string ext = filename.substr(filename.find(".") + 1);
    if (ext == "ply"){

        std::ifstream file(filename, std::ios::binary);
        if (!file){
            std::cout << "Unable to find file: " << filename << std::endl;
        }
        tp::PlyFile pfile;
        pfile.parse_header(file);

        std::shared_ptr<tp::PlyData> verticies;
        verticies = pfile.request_properties_from_element("vertex", {"x","y","z"});

        pfile.read(file);

        float* v = (float*)verticies->buffer.get();



        for (int i = 0; i<verticies->count*3; i+=3){
            float3 point = {v[i],v[i+1],v[i+2]};
            point = (s*float3({dot(point,xaxis), dot(point,yaxis), dot(point,zaxis)}) + t);
            float x = point.x;
            float y = point.y;
            float z = point.z;

            if (x<minx) minx = x;
            if (y<miny) miny = y;
            if (z<minz) minz = z;
            if (x>maxx) maxx = x;
            if (y>maxy) maxy = y;
            if (z>maxz) maxz = z;
        }
        std::cout << "Min: " << minx << "," << miny << "," << minz << std::endl;
        std::cout << "Max: " << maxx << "," << maxy << "," << maxz << std::endl;
        file.close();
    }
    else if (ext == "obj"){
        to::attrib_t attrib;
        std::vector<to::shape_t> shapes;
        std::vector<to::material_t> materials;

        to::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filename.c_str());

        for (size_t sh = 0; sh < shapes.size(); sh++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[sh].mesh.num_face_vertices.size(); f++) {
                //assume triangle
                for (size_t v = 0; v < 3; v++) {
                    // access to vertex
                    to::index_t idx = shapes[sh].mesh.indices[index_offset + v];
                    to::real_t vx = attrib.vertices[3*idx.vertex_index+0];
                    to::real_t vy = attrib.vertices[3*idx.vertex_index+1];
                    to::real_t vz = attrib.vertices[3*idx.vertex_index+2];
                    float3 point = {vx,vy,vz};
                    point = (s*float3({dot(point,xaxis), dot(point,yaxis), dot(point,zaxis)}) + t);

                    float x = point.x;
                    float y = point.y;
                    float z = point.z;

                    if (x<minx) minx = x;
                    if (y<miny) miny = y;
                    if (z<minz) minz = z;
                    if (x>maxx) maxx = x;
                    if (y>maxy) maxy = y;
                    if (z>maxz) maxz = z;
                    //std::cout << x << std::endl;
                }
                index_offset += 3;
            }
        }
        std::cout << "Min: " << minx << "," << miny << "," << minz << std::endl;
        std::cout << "Max: " << maxx << "," << maxy << "," << maxz << std::endl;

    }
    return 0;
}
