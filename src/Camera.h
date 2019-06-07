#pragma once

#include "float3.h"

typedef struct _Camera{
    float3 location;
    float3 looking_at;
    float aperature;
    float lens_radius;
} Camera;
