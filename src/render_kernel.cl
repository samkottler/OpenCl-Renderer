#include "Camera.h"
#include "Material.h"
#include "Ray.h"
#include "Triangle.h"

typedef struct _dat{
    float t;
    float3 normal;
    int mat;
} HitData;

//copied from http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html
uint rand(uint2* state){
    enum { A=4294883355U};
    uint x=(*state).x, c=(*state).y;  // Unpack the state
    uint res=x^c;                     // Calculate the result
    uint hi=mul_hi(x,A);              // Step the RNG
    x=x*A+c;
    c=hi+(x<c);
    *state=(uint2)(x,c);              // Pack the state back up
    return res;                       // Return the next result
}

float intersect(Triangle tri, Ray ray){
    float3 tvec = ray.origin - tri.vert0;
    float3 pvec = cross(ray.direction, tri.vert2 - tri.vert0);
    float det = dot(tri.vert1 - tri.vert0, pvec);
        
    det = native_divide(1.0f,det);

    float u = dot(tvec, pvec)*det;
    if (u < 0 || u > 1)
	return -1e20;

    float3 qvec = cross(tvec, tri.vert1 - tri.vert0);

    float v = dot(ray.direction, qvec) * det;

    if (v < 0 || (u+v) > 1)
        return -1e20;

    return dot(tri.vert2 - tri.vert0, qvec) * det;
}

bool intersect_scene(global Triangle* triangles, int num_tris, Ray ray, HitData* dat){
    float t = 1e20;
    dat->t = t;
    float d;
    int id;
    for (int i = 0; i<num_tris; ++i){
	if ((d=intersect(triangles[i],ray)) && d<t && d>0){
	    t=d;
	    id = i;
	}
    }
    if (t<dat->t){
	float3 v0 = triangles[id].vert0;
	float3 v1 = triangles[id].vert1;
	float3 v2 = triangles[id].vert2;
	dat->t = t;
	dat->normal = normalize(cross(v2-v0, v1-v0));
	dat->mat = triangles[id].material;
    }
    return t < 1e19;
}

float3 trace(global Triangle* triangles, int num_tris, Ray ray, global Material* materials, uint2* rand_state){
    float3 color = (float3)(0.0,0.0,0.0);
    float3 mask = (float3)(1.0,1.0,1.0);
    for (int bounces = 0; bounces < 5; ++bounces){
	HitData dat;
	if(intersect_scene(triangles, num_tris, ray, &dat)){
	    Material mat = materials[dat.mat];
	    ray.origin = ray.origin + dat.t*ray.direction + 0.01f*dat.normal;
	    float theta = 2*M_PI*((float)rand(rand_state)/(float)0xffffffffU);
	    float cosphi = ((float)rand(rand_state)/(float)0xffffffffU);
	    float sinphi = sqrt(1-cosphi*cosphi);
	   
	    float3 w = dat.normal;
	    float3 u = normalize(cross(fabs(w.x)>0.0001?(float3)(0,1,0):(float3)(1,0,0),w));
	    float3 v = cross(w,u);
	    ray.direction = normalize(u*cos(theta)*sinphi + v*sin(theta)*sinphi + w*cosphi);
	    color += mask*mat.emission;
	    mask = mask*mat.color;
	}
	else{
	    float t = (ray.direction.y + 1)/2;
	    return color + mask*((float3)(1,1,1)*(1-t) + (float3)(0.5,0.7,1)*t);
	}
    }

    return color;
}

void kernel render(global float3* image, global Triangle* triangles, int num_tris, global Material* materials, Camera camera){
    int x = get_global_id(0);
    int y = get_global_id(1);
    int width = get_global_size(0);
    int height = get_global_size(1);
    uint2 rand_state = (uint2)(as_uint(image[(height-y-1)*width+x].x),as_uint(image[(height-y-1)*width+x].y));
    rand(&rand_state);
    
    //camera space unit basis vectors
    float3 w = normalize(camera.location - camera.looking_at);
    float3 u = cross((float3)(0,1,0), w);
    float3 v = cross(w,u);

    float focal_length = length(camera.location - camera.looking_at);
    float screen_height = native_tan(camera.aperature/2);
    float screen_width = screen_height*width/height;

    float3 screen_corner = camera.location - screen_width*focal_length*u - screen_height*focal_length*v - focal_length*w;
    float3 horiz = 2*screen_width*focal_length*u;
    float3 vert = 2*screen_height*focal_length*v;
    
    Ray ray;
    ray.origin = camera.location;
    ray.direction = normalize(screen_corner + horiz*(float)x/(float)width + vert*(float)y/(float)height - ray.origin);

    float3 color = (float3)(0,0,0);
    for (int sample = 0; sample < SAMPLES; ++sample){
	color += trace(triangles, num_tris, ray, materials, &rand_state);
    }
    
    image[(height-y-1)*width+x] = color/SAMPLES;
}
