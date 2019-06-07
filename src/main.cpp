#include <chrono>
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
    std::cout << "  -s <num>    Trace a maximum of <num> paths for each pixel." << std::endl;
    exit(0);
}

int main(int argc, char** argv){
    std::string save_file = "test.png";
    std::string scene_file = "cornel_box.scene";
    int samples = 100;
    int width = 512;
    int height = 384;
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
	if (strcmp(argv[i], "-s") == 0){
	    if (i+1 < argc){
		samples = atoi(argv[i+1]);
	    }
	    else{
		std::cout << "No maximum samples specified" << std::endl;
		usage(argv[0]);
	    }
	}
    }

    std::chrono::time_point<std::chrono::system_clock> t0,t1,t2,t3;

    t0 = std::chrono::system_clock::now();

    Scene scene(scene_file);
    Renderer renderer("src/render_kernel.cl", width, height, samples);
    std::clog << "Image info:" << std::endl;
    std::clog << "  Width:     " << width << std::endl;
    std::clog << "  Hiehgt:    " << height << std::endl;
    std::clog << "  Samples:   " << samples << std::endl;
    std::clog << "  Triangles: " << scene.triangles.size() << std::endl;

    t1 = std::chrono::system_clock::now();
    
    renderer.render(scene);

    t2 = std::chrono::system_clock::now();
    
    renderer.save_image(save_file);

    t3 = std::chrono::system_clock::now();

    std::chrono::duration<double> init_time = t1 - t0;
    std::chrono::duration<double> kernel_time = t2 - t1;
    std::chrono::duration<double> post_time = t3 - t2;

    double is = init_time.count();
    double ks = kernel_time.count();
    double ps = post_time.count();
    int im = is/60;
    int km = ks/60;
    int pm = ps/60;
    is -= 60*im;
    ks -= 60*km;
    ps -= 60*pm;
    
    std::clog << std::endl;
    std::clog << "Initialization time: " << im << "m" << is << "s" << std::endl;
    std::clog << "Render time: " << km << "m" << ks << "s" << std::endl;
    std::clog << "Post process time: " << pm << "m" << ps << "s" << std::endl;
    
    return 0;
}
