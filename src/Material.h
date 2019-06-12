#pragma once

#include "float3.h"

typedef enum _BRDF{
	   LAMBERTIAN,
	   COOK_TORRANCE
} BRDF;

typedef struct _mat{
    float3 color;
    float3 emission;
    BRDF type;
    float alpha;
} Material;
