#pragma once

#include "float3.h"

typedef struct _Camera{
    float3 location;
    float3 looking_at;
    float aperture;
    float lens_radius;
} Camera;
