#include "Box.h"
#include "Camera.h"
#include "GPU_BVHnode.h"
#include "Material.h"
#include "Ray.h"
#include "Triangle.h"

#define RAND_MAX (0x800000U)

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
    return res&0x7fffff;              // Return the next result
                                      // modified to not ever get 1
}

float cosTheta(float3 w){
    return w.z;
}

float sinTheta(float3 w){
    return sqrt(max(0.0, 1.0 - w.z*w.z));
}

float tanTheta(float3 w){
    return sinTheta(w)/cosTheta(w);
}

float Lambda(float3 w, Material mat){
    float absTanTheta = fabs(tanTheta(w));
    if (absTanTheta > 1e20)
        return 0;
    float a = 1.0/(mat.alpha*absTanTheta);
    if (a>=1.6)
        return 0;
    return (1 - 1.259*a + 0.396*a*a) / (3.535*a + 2.181*a*a);
}

float G(float3 in, float3 out, Material mat){
    return 1.0/(1+Lambda(in, mat)+Lambda(out, mat));
}

float F(float etaI, float etaT, float cosThetaI){
    float sinThetaI = sqrt(1-cosThetaI*cosThetaI);
    float sinThetaT = etaI/etaT*sinThetaI;
    if (sinThetaT >= 1)
        return 1;
    float cosThetaT = sqrt(1-sinThetaT*sinThetaT);
    float r_parallel = ((etaT*cosThetaI) - (etaI*cosThetaT)) / ((etaT*cosThetaI) + (etaI*cosThetaT));
    float r_perpendicular = ((etaI*cosThetaI) - (etaT*cosThetaT)) / ((etaI*cosThetaI) + (etaT*cosThetaT));
    return (r_parallel*r_parallel + r_perpendicular*r_perpendicular) / 2;
}

float3 global_to_local(float3 normal, float3 vec){
    float3 z = normal;
    float3 y = normalize(cross(z,(fabs(z.z - 1) < 0.0001)?(float3)(1,0,0):(float3)(0,0,1)));
    float3 x = cross(y,z);

    return (float3)(dot(x,vec), dot(y, vec), dot(z,vec));
}

float3 local_to_global(float3 normal, float3 vec){
    float3 z = normal;
    float3 y = normalize(cross(z,(fabs(z.z - 1) < 0.0001)?(float3)(1,0,0):(float3)(0,0,1)));
    float3 x = cross(y,z);

    float3 a1 = (float3)(x.x, y.x, z.x);
    float3 a2 = (float3)(x.y, y.y, z.y);
    float3 a3 = (float3)(x.z, y.z, z.z);

    return (float3)(dot(a1,vec), dot(a2, vec), dot(a3, vec));
}

float3 get_direction(float3 normal, float3 in, Material mat, float* bxdf, uint2* rand_state, int* transmitted, Material current){
    float3 win = -global_to_local(normal, in);
    float ref_type = (float)(rand(rand_state))/(float)RAND_MAX;
    float etaI;
    float etaT;
    float3 nl;
    if (win.z < 0){ // leaving
        nl = (float3)(0,0,-1);
        etaI = mat.ref_idx;
        etaT = current.ref_idx;
    }
    else{ // entering
        nl = (float3)(0,0,1);
        etaI = current.ref_idx;
        etaT = mat.ref_idx;
    }
    if (ref_type > F(etaI, etaT, fabs(cosTheta(win)))){
        *transmitted = 1;
        *bxdf = 1;
        float dt = fabs(cosTheta(win));
        float ratio = etaI/etaT;
        float disc = 1.0 - ratio*ratio*(1-dt*dt);
        float3 refracted = ratio*(-win + dt*nl) - sqrt(disc)*nl;
        return local_to_global(normal, refracted);
    }
    float phi = 2*M_PI*((float)rand(rand_state)/(float)RAND_MAX);
    float xi = (float)(rand(rand_state))/(float)RAND_MAX;
    if(mat.type == LAMBERTIAN){
        float costheta = acos(xi)*2/M_PI;
        float sintheta = sqrt(1-costheta*costheta);
        *bxdf = costheta*costheta/M_PI*2/sqrt(1-xi*xi);
        return local_to_global(normal, (float3)(sintheta*cos(phi), sintheta*sin(phi), costheta));
    }
    else{ // importance sampling based on pbrt
        float tan2theta = -mat.alpha*mat.alpha*log(xi);
        float costheta = 1/sqrt(1+tan2theta);
        float sintheta = sqrt(1-costheta*costheta);

        float3 w_half = (float3)(sintheta*cos(phi), sintheta*sin(phi), costheta);
        if (win.z*w_half.z < 0) w_half = -w_half;

        float3 wout = -win + 2*w_half*dot(win,w_half);

        float cout = cosTheta(wout);
        float cin = cosTheta(win);
        if( cout*cin < 0){
            *bxdf = 0;
        }
        else
            *bxdf = G(win, wout, mat)/cin/fabs(cosTheta(w_half))*dot(win,w_half);

        return local_to_global(normal, wout);
    }
}

// return min and max components of a vector
float3 minf3(float3 a, float3 b){
    return (float3)(a.x<b.x?a.x:b.x, a.y<b.y?a.y:b.y, a.z<b.z?a.z:b.z);
}
float3 maxf3(float3 a, float3 b){
    return (float3)(a.x>b.x?a.x:b.x, a.y>b.y?a.y:b.y, a.z>b.z?a.z:b.z);
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
                    if(d<t && d>0.000001){
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
    Material stack[10];
    int stack_idx = 0;
    stack[0].ref_idx = 1;
    stack[0].attenuation = (float3)(0, 0, 0);
    for (int bounces = 0; bounces < 10; ++bounces){
        HitData dat;
        if(intersect_scene(bvh, triangles, ray, &dat)){
            Material mat = materials[dat.mat];
            ray.origin = ray.origin + dat.t*ray.direction;

            float3 atten = stack[stack_idx].attenuation;

            float bxdf;
            int transmitted = 0;
            float3 new_direction = get_direction(dat.normal, ray.direction, mat, &bxdf, rand_state, &transmitted, stack[stack_idx]);

            if (transmitted){
                if (dot(dat.normal, ray.direction) < 0)
                    stack[++stack_idx] = mat;
                else
                    stack_idx--;
            }
            else if (dot(dat.normal, ray.direction) < 0)
                ray.origin += 0.000001f*dat.normal;

            float r = exp(-atten.x*dat.t);
            float g = exp(-atten.y*dat.t);
            float b = exp(-atten.z*dat.t);
            mask = mask*(float3)(r,g,b);

            color += mask*mat.emission;
            mask = mask*mat.color*bxdf;
            ray.direction = new_direction;
        }
        else{
            float t = (ray.direction.y + 1)/2;
            return color + mask*((float3)(1,1,1)*(1-t) + (float3)(0.5,0.7,1)*t);
        }
        if (mask.x + mask.y + mask.z < 0.01) break;
    }

    return color;
}

void kernel render(global float3* image, global uint2* seeds, global GPU_BVHnode* bvh, global Triangle* triangles, global Material* materials, Camera camera, int samples){
    int x = get_global_id(0);
    int y = get_global_id(1);
    int width = get_global_size(0);
    int height = get_global_size(1);
    uint2 rand_state = seeds[(height-y-1)*width+x];
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
    for (int sample = 0; sample < samples; ++sample){
        Ray ray;

        float theta = 2*M_PI*(float)rand(&rand_state)/(float)RAND_MAX;
        float rad = camera.lens_radius*(float)rand(&rand_state)/(float)RAND_MAX;
        ray.origin = camera.location + rad*(u*cos(theta) + v*sin(theta));

        float xs = (float)rand(&rand_state)/(float)RAND_MAX;
        float ys = (float)rand(&rand_state)/(float)RAND_MAX;

        ray.direction = normalize(screen_corner + horiz*(float)(x+xs)/(float)width + vert*(float)(y+ys)/(float)height - ray.origin);


        color += trace(bvh, triangles, ray, materials, &rand_state);
    }

    image[(height-y-1)*width+x] += color;
    seeds[(height-y-1)*width+x] = rand_state;

}
