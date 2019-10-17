#include <iostream>
#include <float.h>
#include <vector>

#include "BVH.hpp"
#include "GPU_BVHnode.h"
#include "Triangle.h"

// return min and max components of a vector
inline  float3 minf3(float3 a, float3 b){
    return {a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z};
}
inline float3 maxf3(float3 a, float3 b){
    return {a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z};
}

BVHnode* BVH::recurse(std::vector<BBoxTemp*>& working, int depth = 0){
    ++numNodes;
    if (working.size() < 4){ // if only 4 triangles left
        BVHleaf* leaf = new BVHleaf;
        for(int i = 0; i< working.size(); ++i)
            leaf->triangles.push_back(working[i]->triangle);
        return leaf;
    }
    float3 min = {FLT_MAX,FLT_MAX,FLT_MAX};
    float3 max = {-FLT_MAX,-FLT_MAX,-FLT_MAX};

    // calculate bounds for current working list
    for(uint i = 0; i<working.size(); ++i){
        BBoxTemp* v = working[i];
        min = minf3(min, v->min);
        max = maxf3(max, v->max);
    }

    //approxomate SA of triangle by size of bounding box
    float side1 = max.x - min.x;
    float side2 = max.y - min.y;
    float side3 = max.z - min.z;

    float min_cost = working.size() * (side1*side2 +
        side2*side3 +
        side3*side1);
        float best_split = FLT_MAX; // best value along axis

    int best_axis = -1; // best axis

        // 0 = X-axis, 1 = Y-axis, 2=Z-axis
    for(int i = 0; i< 3; ++i){ //check all three axes
        int axis = i;
        float start, stop, step;
        if(axis == 0){
            start = min.x;
            stop = max.x;
        }
        else if (axis == 1){
            start = min.y;
            stop = max.y;
        }
        else{
            start = min.z;
            stop = max.z;
        }

        // if box is too thin in this direction
        if (abs(stop - start) < 1e-4)
            continue;

        // check discrete number of different splits on each axis
        // number gets smaller as we get farther into bvh and presumably smaller differences
        step = (stop - start) / (1024.0 / (depth + 1));

        // determine how good each plane is for splitting
        for(float test_split = start + step; test_split < stop-step; test_split += step){
            float3 lmin = {FLT_MAX,FLT_MAX,FLT_MAX};
            float3 lmax = {-FLT_MAX,-FLT_MAX,-FLT_MAX};

            float3 rmin = {FLT_MAX,FLT_MAX,FLT_MAX};
            float3 rmax = {-FLT_MAX,-FLT_MAX,-FLT_MAX};

            int lcount = 0;
            int rcount = 0;

            for(uint j = 0; j<working.size(); ++j){
                BBoxTemp* v = working[j];
                float val;
                // use triangle center to determine which side to put it in
                if (axis == 0) val = v->center.x;
                else if (axis == 1) val = v->center.y;
                else val = v->center.z;

                if(val < test_split){
                    lmin = minf3(lmin, v->min);
                    lmax = maxf3(lmax, v->max);
                    lcount++;
                }
                else{
                    rmin = minf3(rmin, v->min);
                    rmax = maxf3(rmax, v->max);
                    rcount++;
                }
            }

            if (lcount <= 1 || rcount <=1) continue;

            float lside1 = lmax.x - lmin.x;
            float lside2 = lmax.y - lmin.y;
            float lside3 = lmax.z - lmin.z;

            float rside1 = rmax.x - rmin.x;
            float rside2 = rmax.y - rmin.y;
            float rside3 = rmax.z - rmin.z;

            float lsurface = lside1*lside2 + lside2*lside3 + lside3*lside1;
            float rsurface = rside1*rside2 + rside2*rside3 + rside3*rside1;

            float total_cost =  lsurface*lcount + rsurface*rcount;
            if (total_cost < min_cost){ // if this split is better, update stuff
                min_cost = total_cost;
                best_split = test_split;
                best_axis = axis;
            }
        }
    }
    // if no split is better, just add a leaf node
    if (best_axis == -1){
        BVHleaf* leaf = new BVHleaf;
        for(int i = 0; i< working.size(); ++i)
            leaf->triangles.push_back(working[i]->triangle);
        return leaf;
    }

    // otherwise, create left and right working lists and call function recursively
    std::vector<BBoxTemp*> left;
    std::vector<BBoxTemp*> right;
    float3 lmin = {FLT_MAX,FLT_MAX,FLT_MAX};
    float3 lmax = {-FLT_MAX,-FLT_MAX,-FLT_MAX};
    float3 rmin = {FLT_MAX,FLT_MAX,FLT_MAX};
    float3 rmax = {-FLT_MAX,-FLT_MAX,-FLT_MAX};

    for(uint i = 0; i<working.size(); ++i){
        BBoxTemp* v = working[i];
        float val;
        if (best_axis == 0) val = v->center.x;
        else if (best_axis == 1) val = v->center.y;
        else val = v->center.z;
        if(val < best_split){
            left.push_back(v);
            lmin = minf3(lmin, v->min);
            lmax = maxf3(lmax, v->max);
        }
        else{
            right.push_back(v);
            rmin = minf3(rmin, v->min);
            rmax = maxf3(rmax, v->max);
        }
    }

    //create left and right child nodes
    BVHinner* inner = new BVHinner;
    inner->left = recurse(left, depth+1);
    inner->left->min = lmin;
    inner->left->max = lmax;

    inner->right = recurse(right, depth+1);
    inner->right->min = rmin;
    inner->right->max = rmax;

    return inner;
}

// convert naive bvh to memory friendly bvh
void BVH::populate_GPU_BVHnode(const Triangle* first, BVHnode* root, unsigned int& boxoffset, unsigned int& trioffset){
    int curr = GPU_BVH.size();
    GPU_BVHnode new_node;
    new_node.min = root->min;
    new_node.max = root->max;
    GPU_BVH.push_back(new_node);
    if(!root->is_leaf()){
        BVHinner* p = dynamic_cast<BVHinner*>(root);
        int loffset = ++boxoffset;
        populate_GPU_BVHnode(first, p->left, boxoffset, trioffset);
        int roffset = ++boxoffset;
        populate_GPU_BVHnode(first, p->right, boxoffset, trioffset);
        GPU_BVH[curr].u.inner.left = loffset;
        GPU_BVH[curr].u.inner.right = roffset;
        //std::cout << loffset << " " << roffset << std::endl;
    }
    else{
        BVHleaf* p = dynamic_cast<BVHleaf*>(root);
        uint count = (uint)p->triangles.size();
        GPU_BVH[curr].u.leaf.count = 0x80000000 | count;
        // use highest bit to indicate type of node because polymorphism is bad on gpu
        GPU_BVH[curr].u.leaf.offset = trioffset;
        for(int i = 0; i<p->triangles.size(); ++i){
            ordered.push_back(*(p->triangles[i]));
            trioffset++;
        }
    }
    delete root;
}

BVH::BVH(std::vector<Triangle>& triangles){
    std::vector<BBoxTemp> working(triangles.size());
    std::vector<BBoxTemp*> working_p(triangles.size());
    float3 min={FLT_MAX, FLT_MAX, FLT_MAX};
    float3 max={-FLT_MAX, -FLT_MAX, -FLT_MAX};

    std::clog << "Gathering box info..." << std::endl;

    for (uint i = 0; i<triangles.size(); ++i){
        const Triangle& triangle = triangles[i];

        BBoxTemp& b = working[i];
        b.triangle = &triangle;

        b.min = minf3(b.min, triangle.vert0);
        b.min = minf3(b.min, triangle.vert1);
        b.min = minf3(b.min, triangle.vert2);

        b.max = maxf3(b.max, triangle.vert0);
        b.max = maxf3(b.max, triangle.vert1);
        b.max = maxf3(b.max, triangle.vert2);

        min = minf3(min, b.min);
        max = maxf3(max, b.max);

        b.center = 0.5f * (b.max + b.min);

        working_p[i] = &b;
    }

    std::clog << "Creating BVH..." << std::endl;

    root = recurse(working_p);
    root->min = min;
    root->max = max;

    std::vector<BBoxTemp>().swap(working);
    std::vector<BBoxTemp*>().swap(working_p);
    ordered = std::vector<Triangle>(triangles.size());
    ordered.clear();
    GPU_BVH = std::vector<GPU_BVHnode>(numNodes);
    GPU_BVH.clear();

    std::clog << "Consolidating BVH..." << std::endl;

    uint trioffset = 0;
    uint boxoffset = 0;

    populate_GPU_BVHnode(triangles.data(), root, boxoffset, trioffset);
    std::vector<Triangle>().swap(triangles);
}
