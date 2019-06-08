#pragma once

#include <float.h>
#include <vector>

#include "GPU_BVHnode.h"
#include "Triangle.h"

// bvh interface on host
struct BVHnode{
    float3 min;
    float3 max;
    virtual bool is_leaf() = 0;
};

// intermediate node. points to left and right
struct BVHinner: BVHnode {
    BVHnode* left;
    BVHnode* right;
    virtual bool is_leaf(){return false;}
};

// leaf node in tree. contains list of triangles
struct BVHleaf: BVHnode{
    std::vector<Triangle> triangles;
    virtual bool is_leaf(){return true;}
};

// triangles that haven't been added to bvh yet 
struct BBoxTemp{
    float3 min;
    float3 max;
    float3 center;
    Triangle triangle;
    BBoxTemp() :
	min({FLT_MAX, FLT_MAX, FLT_MAX}),
	max({-FLT_MAX, -FLT_MAX, -FLT_MAX})
    {}
};

class BVH{
private:
    BVHnode* root;
    BVHnode* recurse(std::vector<BBoxTemp> working, int depth);
    void populate_GPU_BVHnode(const Triangle* first, BVHnode* root, unsigned int& boxoffset, unsigned int& trioffset);
public:
    std::vector<GPU_BVHnode> GPU_BVH;
    std::vector<Triangle> ordered;
    BVH(std::vector<Triangle> triangles);
    BVH(){}
};
