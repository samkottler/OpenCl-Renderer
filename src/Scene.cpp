#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ios>

#include "error.hpp"
#include "float3.h"
#include "Scene.hpp"
#include "Material.h"

void Scene::load_materials(std::string filename){
    std::ifstream material_file(filename, std::ios::in);
    std::string line;
    int line_num = 0;
    std::string mat_name;
    float3 color = {0,0,0};
    float3 emission = {0,0,0};
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
	if (type == "done"){
	    if (mat_name == "")
		print_error(filename + ":" + std::to_string(line_num) + ": No material to end");
	    material_idx[mat_name] = materials.size();
	    materials.push_back({color, emission});
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
		print_error("No file specified on line " + std::to_string(line_num) + " of " + filename);
	    std::string extension = load_name.substr(load_name.find(".")+1);
	    if (extension == "materials")
		load_materials(load_name);
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
}
