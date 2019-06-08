#pragma once

#ifdef __cplusplus
#include <CL/cl.hpp>
using float3 = cl_float3;
float dot(float3 a, float3 b);
float3 cross(float3 a, float3 b);
float3 operator*(float s, float3 v);
float3 operator+(float3 a, float3 b);
#endif
