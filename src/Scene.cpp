#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ios>

#include <cmath>

#include "BVH.hpp"
#include "Camera.h"
#include "error.hpp"
#include "float3.h"
#include "Scene.hpp"
#include "Material.h"

void Scene::load_materials(std::string filename){
    std::ifstream material_file(filename, std::ios::in);
    std::string line;
    if(!material_file)
        print_error("Unable to find file " + filename);
    int line_num = 0;
    std::string mat_name;
    float3 color = {0,0,0};
    float3 emission = {0,0,0};
    BRDF brdf = LAMBERTIAN;
    float alpha;
    float idx = 0;
    float3 attenuation = {0,0,0};
    while (getline(material_file, line)){
        line_num++;
        std::istringstream str(line);
        std::string type;
        str>>type;
        if (type == "material"){
            str>>mat_name;
            if (mat_name == ""){
                print_error(filename + ":" + std::to_string(line_num) + ": Material has no name");
            }
            idx = 0;
        }
        if (type == "c"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material defined");
            float r,g,b = -1e20;
            str >> r;
            str >> g;
            str >> b;
            if (b < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": Unable to create color");
            color = {r,g,b};
        }
        if (type == "e"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material defined");
            float r,g,b = -1e20;
            str >> r;
            str >> g;
            str >> b;
            if (b < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": Unable to create emission");
            emission = {r,g,b};
        }
        if (type == "t"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material defined");
            std::string brdfstr;
            str >> brdfstr;
            if (brdfstr == "lambertian")
                brdf = LAMBERTIAN;
            else if (brdfstr == "cook-torrance")
                brdf = COOK_TORRANCE;
            else
                print_error(filename + ":" + std::to_string(line_num) + ": Couldn't understand brdf type");
        }
        if (type == "a"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material defined");
            alpha = -1e20;
            str >> alpha;
            if(alpha < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": No alpha given");
        }
        if (type == "at"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material defined");
            float r,g,b = -1e20;
            str >> r;
            str >> g;
            str >> b;
            if (b < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": Unable to create attenuation");
            attenuation = {r,g,b};
        }
        if (type == "ri"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material defined");
            idx = -1e20;
            str >> idx;
            if(idx < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": No refraction index given");
        }
        if (type == "done"){
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material to end");
            material_idx[mat_name] = materials.size();
            materials.push_back({color, emission, brdf, alpha, idx, attenuation});
        }
    }
}

void Scene::load_camera(std::string filename){
    std::ifstream camera_file(filename, std::ios::in);
    std::string line;
    if (!camera_file)
        print_error("Unable to find file " + filename);
    int line_num = 0;

    float3 from;
    float3 to;
    float aperture;
    float lens_radius;
    while(getline(camera_file, line)){
        line_num++;
        std::istringstream str(line);
        std::string type;
        str>>type;
        if (type == "f"){
            float x,y,z=-1e20;
            str >> x;
            str >> y;
            str >> z;
            if (z < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": Unable to creat location");
            from = {x,y,z};
        }
        if (type == "t"){
            float x,y,z=-1e20;
            str >> x;
            str >> y;
            str >> z;
            if (z < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": Unable to creat focal point");
            to = {x,y,z};
        }
        if (type == "a"){
            aperture = -1e20;
            str >> aperture;
            if (aperture < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": No aperture specified");
        }
        if (type == "r"){
            lens_radius = -1e20;
            str >> lens_radius;
            if (lens_radius < -1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": No lens radius specified");
        }
    }
    camera = {from, to, (float)(M_PI*aperture/180), lens_radius};
}

void Scene::load_ply(std::string filename, int mat_idx, float3 translate, float scale, float3 xaxis, float3 yaxis){
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

    std::ifstream file(filename, std::ios::in);
    if (!file)
        print_error("Unable to find file: " + filename);
    std::string line;
    uint num_verts, num_tris;
    bool in_header = true;
    std::vector<float3> verts;
    while (getline(file, line)){
        //first get all header info so we know how many verts and tris
        if (in_header){
            if (line.substr(0,14) == "element vertex"){
                std::istringstream str(line);
                std::string w;
                str >> w;
                str >> w;
                str >> num_verts;
            }
            else if (line.substr(0, 12) == "element face"){
                std::istringstream str(line);
                std::string w;
                str >> w;
                str >> w;
                str >> num_tris;
            }
            else if (line.substr(0, 10) == "end_header")
                in_header = false;
        }
        else{ // read all verts
            if (num_verts){
                num_verts--;
                float x, y, z;
                std::istringstream str_in(line);
                str_in >> x >> y >> z;
                float3 point = {x,y,z};
                verts.push_back(scale*float3({dot(point,xaxis), dot(point,yaxis), dot(point,zaxis)}) + translate);
            }
            else if (num_tris){ // once we have all verts, read triangles
                num_tris--;
                uint dummy, idx1, idx2, idx3;
                std::istringstream str(line);
                str >> dummy >> idx1 >> idx2 >> idx3;
                float3 v0 = verts[idx1];
                float3 v1 = verts[idx2];
                float3 v2 = verts[idx3];
                triangles.push_back({v0, v1, v2, mat_idx});
            }
        }
    }
}

Scene::Scene(std::string filename){
    std::clog << "Reading scene..." << std::endl;
    std::ifstream scene_file(filename, std::ios::in);
    std::string line;
    if (!scene_file)
        print_error("Unable to find file " + filename);
    int line_num = 0;
    int current_material=-1;
    std::vector<float3> verticies;
    while(getline(scene_file, line)){
        line_num++;
        std::istringstream str(line);
        std::string type;
        str>>type;
        if (type == "load"){
            std::string load_name;
            str >> load_name;
            if (load_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No file specified");
            std::string extension = load_name.substr(load_name.find(".")+1);
            if (extension == "materials")
                load_materials(load_name);
            else if (extension == "camera")
                load_camera(load_name);
            else if (extension == "ply"){
                if (current_material == -1)
                    print_error(filename + ":" + std::to_string(line_num) + ": No material selected");
                std::string params = str.str().substr(str.str().find(".") + 5);
                float3 t,x,y;
                float s;
                if (sscanf(params.c_str(), "{%f, %f, %f} %f {%f, %f, %f} {%f, %f, %f}",
                &t.x, &t.y, &t.z, &s, &x.x, &x.y, &x.z, &y.x, &y.y, &y.z) != 10)
                    print_error(filename + ":" + std::to_string(line_num) + ": Incorrect parameters");
                load_ply(load_name, current_material, t, s, x, y);
            }
            else
                print_error(filename + ":" + std::to_string(line_num) + ": Extension not recognized");
        }
        if (type == "m"){
            std::string mat_name;
            str>>mat_name;
            if (mat_name == "")
                print_error(filename + ":" + std::to_string(line_num) + ": No material specified");
            if (material_idx.count(mat_name) == 0)
                print_error(filename + ":" + std::to_string(line_num) + ": Material " + mat_name + " does not exist");
            current_material = material_idx[mat_name];
        }
        if (type == "v"){
            float x,y,z=-1e20;
            str >> x;
            str >> y;
            str >> z;
            if (z<-1e19)
                print_error(filename + ":" + std::to_string(line_num) + ": Unable to create vertex");
            verticies.push_back({x,y,z});
        }
        if (type == "f"){
            if (current_material == -1)
                print_error(filename + ":" + std::to_string(line_num) + ": No material selected");
            int v0, v1, v2 = -1;
            str >> v0;
            str >> v1;
            str >> v2;
            triangles.push_back({verticies[v0],verticies[v1],verticies[v2], current_material});
        }
    }
    bvh = BVH(triangles);
}
