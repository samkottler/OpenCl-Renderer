#include <iostream>
#include <string>

#include <cstring>

#include "Renderer.hpp"
#include "Scene.hpp"

void usage(std::string executable){
    std::cout << "Usage: " << executable << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i <file>   Render scene described in <file>." << std::endl;
    std::cout << "  -h          Display this message." << std::endl;
    std::cout << "  -o <file>   Save the output image to <file>." << std::endl;
}

int main(int argc, char** argv){
    std::string save_file = "test.png";
    std::string scene_file = "test.scene";
    for (int i = 1; i< argc; ++i){
	if (strcmp(argv[i], "-o") == 0){
	    if (i+1 < argc){
		save_file = std::string(argv[i+1]);
		++i;
	    }
	    else{
		std::cout << "No output file specified" << std::endl;
		usage(argv[0]);
	    }
	}
	if (strcmp(argv[i], "-h") == 0){
	    usage(argv[0]);
	}
	if (strcmp(argv[i], "-i") == 0){
	    if (i+1 < argc){
		scene_file = std::string(argv[i+1]);
		++i;
	    }
	    else{
		std::cout << "No scene file specified" << std::endl;
		usage(argv[0]);
	    }
	}
    }

    Scene scene(scene_file);
    
    Renderer renderer("render_kenel.cl");
    renderer.render(scene);
    renderer.save_image(save_file);
    
    return 0;
}
