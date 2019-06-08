#include "float3.h"

float dot(float3 a, float3 b){
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

float3 cross(float3 a, float3 b){
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

float3 operator*(float s, float3 v){
    return {s*v.x, s*v.y, s*v.z};
}

float3 operator+(float3 a, float3 b){
    return {a.x+b.x, a.y+b.y, a.z+b.z}; 
}
