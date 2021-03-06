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
    virtual ~BVHnode(){}
};

// intermediate node. points to left and right
struct BVHinner: BVHnode {
    BVHnode* left;
    BVHnode* right;
    virtual bool is_leaf(){return false;}
    virtual ~BVHinner(){
        //delete left;
        //delete right;
    }
};

// leaf node in tree. contains list of triangles
struct BVHleaf: BVHnode{
    std::vector<const Triangle*> triangles;
    virtual bool is_leaf(){return true;}
    virtual ~BVHleaf(){}
};

// triangles that haven't been added to bvh yet
struct BBoxTemp{
    float3 min;
    float3 max;
    float3 center;
    const Triangle* triangle;
    BBoxTemp() :
	min({FLT_MAX, FLT_MAX, FLT_MAX}),
	max({-FLT_MAX, -FLT_MAX, -FLT_MAX})
    {}
};

class BVH{
private:
    int numNodes = 0;
    BVHnode* root;
    BVHnode* recurse(std::vector<BBoxTemp*>& working, int depth);
    void populate_GPU_BVHnode(const Triangle* first, BVHnode* root, unsigned int& boxoffset, unsigned int& trioffset);
public:
    std::vector<GPU_BVHnode> GPU_BVH;
    std::vector<Triangle> ordered;
    BVH(std::vector<Triangle>& triangles);
    BVH(){}
};
