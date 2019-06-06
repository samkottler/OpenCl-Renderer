#pragma once

#include <iostream>
#include <string>

void print_error(std::string message){
    std::cerr << "Error: " << message << std::endl;
    exit(1);
}

void print_warning(std::string message){
    std::cerr << "Warning: " << message << std::endl;
}
