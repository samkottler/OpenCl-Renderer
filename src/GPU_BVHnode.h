#pragma once

#include "float3.h"

typedef struct _GPU_BVHnode{
    float3 min;
    float3 max;
    union{
	struct{
	    unsigned int left;
	    unsigned int right;
	} inner;
	struct{
	    unsigned int count;
	    unsigned int offset;
	} leaf;
    }u;
}GPU_BVHnode;
