#include "Box.h"
#include "Camera.h"
#include "GPU_BVHnode.h"
#include "Material.h"
#include "Ray.h"
#include "Triangle.h"

#define RAND_MAX (0xffffffffU)

typedef struct _dat{
    float t;
    float3 normal;
    int mat;
} HitData;

// return min and max components of a vector
float3 minf3(float3 a, float3 b){
    return (float3)(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z);
}
float3 maxf3(float3 a, float3 b){
    return (float3)(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z);
}

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

float intersect_box(Box box, Ray ray){
    if (box.min.x < ray.origin.x && ray.origin.x < box.max.x &&
	box.min.y < ray.origin.y && ray.origin.y < box.max.y &&
	box.min.z < ray.origin.z && ray.origin.z < box.max.z) return -1;
	
	float3 tmin = (box.min - ray.origin) / ray.direction;
	float3 tmax = (box.max - ray.origin) / ray.direction;

	float3 rmin = minf3(tmin,tmax);
	float3 rmax = maxf3(tmin,tmax);

	float minmax = fmin(fmin(rmax.x, rmax.y), rmax.z);
	float maxmin = fmax(fmax(rmin.x, rmin.y), rmin.z);

	if(minmax >= maxmin) return maxmin > 0.000001 ? maxmin : 0;
	else return 0;
}

bool intersect_scene(global GPU_BVHnode* bvh, global Triangle* triangles, Ray ray, HitData* dat){
    int stack[64]; // its reasonable to assume this will be way bigger than neccesary
    int stack_idx = 1;
    stack[0] = 0;
    float d;
    float t = 1e20;
    dat->t = t;
    int id;
    while(stack_idx){
	int boxidx = stack[stack_idx - 1]; // pop off top of stack
	stack_idx --; 
	if(!(bvh[boxidx].u.leaf.count & 0x80000000)){ // inner
	    Box b;
	    b.min = bvh[boxidx].min;
	    b.max = bvh[boxidx].max;
	    if (intersect_box(b,ray)){
		stack[stack_idx++] = bvh[boxidx].u.inner.left; // push right and left onto stack
		stack[stack_idx++] = bvh[boxidx].u.inner.right;
	    }
	} 
	else{ // leaf
	    for (int i = bvh[boxidx].u.leaf.offset;
		 i < bvh[boxidx].u.leaf.offset + (bvh[boxidx].u.leaf.count & 0x7fffffff);
		 i++){ // intersect all triangles in this box
		if ((d = intersect(triangles[i],ray)) && d > -1e19){
		    if(d<t && d>0.001){
			t=d;
			id = i;
		    }
		}
	    }
	}
    }
    if (t<dat->t){
	float3 v0 = triangles[id].vert0;
	float3 v1 = triangles[id].vert1;
	float3 v2 = triangles[id].vert2;
	dat->t = t;
	dat->normal = normalize(cross(v1-v0, v2-v0));
	dat->mat = triangles[id].material;
    }
    return t < 1e19;
}

float3 trace(global GPU_BVHnode* bvh, global Triangle* triangles, Ray ray, global Material* materials, uint2* rand_state){
    float3 color = (float3)(0.0,0.0,0.0);
    float3 mask = (float3)(1.0,1.0,1.0);
    for (int bounces = 0; bounces < 5; ++bounces){
	HitData dat;
	if(intersect_scene(bvh, triangles, ray, &dat)){
	    Material mat = materials[dat.mat];
	    ray.origin = ray.origin + dat.t*ray.direction + 0.01f*dat.normal;
	    float theta = 2*M_PI*((float)rand(rand_state)/(float)RAND_MAX);
	    float cosphi = ((float)rand(rand_state)/(float)RAND_MAX);
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

void kernel render(global float3* image, global GPU_BVHnode* bvh, global Triangle* triangles, global Material* materials, Camera camera){
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
    float screen_height = native_tan(camera.aperture/2);
    float screen_width = screen_height*width/height;

    float3 screen_corner = camera.location - screen_width*focal_length*u - screen_height*focal_length*v - focal_length*w;
    float3 horiz = 2*screen_width*focal_length*u;
    float3 vert = 2*screen_height*focal_length*v;

    float3 color = (float3)(0,0,0);
    for (int sample = 0; sample < SAMPLES; ++sample){
	Ray ray;
	
	float theta = 2*M_PI*(float)rand(&rand_state)/(float)RAND_MAX;
	float rad = camera.lens_radius*(float)rand(&rand_state)/(float)RAND_MAX;
	ray.origin = camera.location + rad*(u*cos(theta) + v*sin(theta));

	float xs = (float)rand(&rand_state)/(float)RAND_MAX;
	float ys = (float)rand(&rand_state)/(float)RAND_MAX;

	ray.direction = normalize(screen_corner + horiz*(float)(x+xs)/(float)width + vert*(float)(y+ys)/(float)height - ray.origin);


	color += trace(bvh, triangles, ray, materials, &rand_state);
    }
    
    image[(height-y-1)*width+x] = color/SAMPLES;
}
