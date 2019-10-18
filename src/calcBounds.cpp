#include <iostream>
#include <fstream>
#include <vector>

#include <CL/cl.hpp>

#include <cstdio>

#include "tinyply.h"
#include "float3.h"

namespace tp = tinyply;

int main(int argc, char** argv){
    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);
    if (!file){
        std::cout << "Unable to find file: " << filename << std::endl;
    }

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


    float3 t = {std::stof(argv[9]), std::stof(argv[10]), std::stof(argv[11])};

    tp::PlyFile pfile;
    pfile.parse_header(file);

    std::shared_ptr<tp::PlyData> verticies;
    verticies = pfile.request_properties_from_element("vertex", {"x","y","z"});

    pfile.read(file);

    float* v = (float*)verticies->buffer.get();

    float minx = 1000, miny = 1000, minz = 1000;
    float maxx = -1000, maxy = -1000, maxz = -1000;

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
    return 0;
}
