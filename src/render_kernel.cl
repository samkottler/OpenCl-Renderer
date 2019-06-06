#include "GPUScene.h"

void kernel render(global float3* image){
    int x = get_global_id(0);
    int y = get_global_id(1);
    int w = get_global_size(0);
    int h = get_global_size(1);
    image[y*w+x] = (float3)((float)x/w,(float)y/h,0);
}
