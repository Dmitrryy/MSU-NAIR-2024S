#include <vector>
#include <chrono>
#include <string>

#include "tracer.hpp"

float2 RayBoxIntersection(float3 ray_pos, float3 ray_dir, float3 boxMin, float3 boxMax)
{
  ray_dir.x = 1.0f/ray_dir.x; // may precompute if intersect many boxes
  ray_dir.y = 1.0f/ray_dir.y; // may precompute if intersect many boxes
  ray_dir.z = 1.0f/ray_dir.z; // may precompute if intersect many boxes

  float lo = ray_dir.x*(boxMin.x - ray_pos.x);
  float hi = ray_dir.x*(boxMax.x - ray_pos.x);
  
  float tmin = std::min(lo, hi);
  float tmax = std::max(lo, hi);

  float lo1 = ray_dir.y*(boxMin.y - ray_pos.y);
  float hi1 = ray_dir.y*(boxMax.y - ray_pos.y);

  tmin = std::max(tmin, std::min(lo1, hi1));
  tmax = std::min(tmax, std::max(lo1, hi1));

  float lo2 = ray_dir.z*(boxMin.z - ray_pos.z);
  float hi2 = ray_dir.z*(boxMax.z - ray_pos.z);

  tmin = std::max(tmin, std::min(lo2, hi2));
  tmax = std::min(tmax, std::max(lo2, hi2));
  
  return float2(tmin, tmax);
}

static inline float3 EyeRayDir(float x, float y, float4x4 a_mViewProjInv)
{
  float4 pos = float4(2.0f*x - 1.0f, 2.0f*y - 1.0f, 0.0f, 1.0f );
  pos = a_mViewProjInv * pos;
  pos /= pos.w;
  return normalize(to_float3(pos));
}

static inline void transform_ray3f(float4x4 a_mWorldViewInv, float3* ray_pos, float3* ray_dir) 
{
  float4 rayPosTransformed = a_mWorldViewInv*to_float4(*ray_pos, 1.0f);
  float4 rayDirTransformed = a_mWorldViewInv*to_float4(*ray_dir, 0.0f);
  
  (*ray_pos) = to_float3(rayPosTransformed);
  (*ray_dir) = to_float3(normalize(rayDirTransformed));
}

float4 RayMarchConstantFog(float tmin, float tmax, float& alpha)
{
  float dt = 0.05f;
	float t  = tmin;
	
	alpha = 1.0f;
	float4 color = float4(0.0f);
	
	while(t < tmax && alpha > 0.01f)
	{
	  float a = 0.025f;
	  color += a*alpha*float4(1.0f,1.0f,0.0f,0.0f);
	  alpha *= (1.0f-a);
	  t += dt;
	}
	
	return color;
}

static inline uint32_t RealColorToUint32(float4 real_color)
{
  float  r = real_color[0]*255.0f;
  float  g = real_color[1]*255.0f;
  float  b = real_color[2]*255.0f;
  float  a = real_color[3]*255.0f;

  uint32_t red   = (uint32_t)r;
  uint32_t green = (uint32_t)g;
  uint32_t blue  = (uint32_t)b;
  uint32_t alpha = (uint32_t)a;

  return red | (green << 8) | (blue << 16) | (alpha << 24);
}

static inline float DE_sphere(float3 pos) {
  const float R = 1.f;
  return std::max(0.0f, length(pos) - R);
}

inline float SDE_Parallelepiped(float3 point, float3 size) {
    float3 d = abs(point) - size;
    return min(max(d.x,max(d.y,d.z)), 0.0f) + length(max(d, float3(0.0f)));
}

// src: https://www.shadertoy.com/view/lljGz3
static inline float2 DE_tetrahedron(float3 pos, int iters) {

const float3 va (  0.0,  0.57735,  0.0 );
const float3 vb (  0.0, -1.0,  1.15470 );
const float3 vc (  1.0, -1.0, -0.57735 );
const float3 vd ( -1.0, -1.0, -0.57735 );

    float a = 0.0;
    float s = 1.0;
    float r = 1.0;
    float dm;
    float3 v;
    for(int i=0; i<iters; i++) {
        float d, t;
        d = dot(pos-va,pos-va);              v=va; dm=d; t=0.0;
        d = dot(pos-vb,pos-vb); if( d<dm ) { v=vb; dm=d; t=1.0; }
        d = dot(pos-vc,pos-vc); if( d<dm ) { v=vc; dm=d; t=2.0; }
        d = dot(pos-vd,pos-vd); if( d<dm ) { v=vd; dm=d; t=3.0; }
        pos = v + 2.0*(pos - v); r*= 2.0;
        a = t + 4.0*a; s*= 4.0;
    }
    
    return float2((sqrt(dm)-1.0)/r, a/s );
}

static inline float DE(float3 pos, float4 &color) {
  float result = 1 / 0.000000001f;
  
  // plate
  // result = std::min(result, SDE_Parallelepiped(pos + float3(0.f, 1.f, 0.f), float3(3.f, 0.1f, 3.f)));
  float d = SDE_Parallelepiped(pos + float3(0.f, 1.f, 0.f), float3(4.f, 0.1f, 4.f));
  if (d < result) {
    result = d;
    color = float4(153.f / 255, 255.f / 255, 255.f / 255, 1.f);
  }

  // fig 1
  float2 dt = DE_tetrahedron(pos, 5);
  if (dt.x < result) {
    result = dt.x;
    float3 color_tmp = 6.2831f * dt.y + float3(0.f, 1.f, 2.f);
    color_tmp = 0.5f + 0.5f * float3(cos(color_tmp.x), cos(color_tmp.y), cos(color_tmp.z));
    color = float4(color_tmp.x, color_tmp.y, color_tmp.z, 1.f);
  }

  // fig 1.2
  dt = DE_tetrahedron(pos + float3(2.f, 0.f, 0.f), 8);
  if (dt.x < result) {
    result = dt.x;
    float3 color_tmp = 6.2831f * dt.y + float3(0.f, 1.f, 2.f);
    color_tmp = 0.5f + 0.5f * float3(cos(color_tmp.x), cos(color_tmp.y), cos(color_tmp.z));
    color = float4(color_tmp.x, color_tmp.y, color_tmp.z, 1.f);
  }

  // fig 1.3
  dt = DE_tetrahedron(pos + float3(-2.f, 0.f, 0.f), 10);
  if (dt.x < result) {
    result = dt.x;
    float3 color_tmp = 6.2831f * dt.y + float3(0.f, 1.f, 2.f);
    color_tmp = 0.5f + 0.5f * float3(cos(color_tmp.x), cos(color_tmp.y), cos(color_tmp.z));
    color = float4(color_tmp.x, color_tmp.y, color_tmp.z, 1.f);
  }

  // fig 2
  d = DE_sphere(pos + float3(1.5f, 0.f, 1.5f));
  if (d < result) {
    result = d;
    color = float4(1.f);
  }
  return result;
}

inline float3 calculateNorm(float3 pos, float eps) {
  float4 ignore;
  const float dx = DE(pos + float3(eps, 0.f, 0.f), ignore) - DE(pos - float3(eps, 0.f, 0.f), ignore);
  const float dy = DE(pos + float3(0.f, eps, 0.f), ignore) - DE(pos - float3(0.f, eps, 0.f), ignore);
  const float dz = DE(pos + float3(0.f, 0.f, eps), ignore) - DE(pos - float3(0.f, 0.f, eps), ignore);
  return normalize(float3{dx, dy, dz});
}

inline float calculateLight(float3 pos, float3 normal, float3 light_dir, float eps, int MAX_ITER, float MAX_DIST) {
  float3 shadowRayPos = pos + normal * eps;
  float3 shadowRayDir = light_dir;
  float4 ignore;
  bool shadowed = false;
  for(uint32_t s = 0; s < MAX_ITER; ++s) {
    const float d = DE(shadowRayPos, ignore);
    if (d < eps) {
      shadowed = true;
      break;
    }
    if (d > MAX_DIST) {
      break;
    }
    shadowRayPos += d * shadowRayDir;
  }

  // Ambient occlusion
  const float4 coeffs(0.01f, 0.05f, 0.1f, 0.2f);
  const float betta = coeffs[0] + coeffs[1] + coeffs[2] + coeffs[3];
  const float alpha = DE(pos + normal * coeffs[0], ignore) + DE(pos + normal * coeffs[1], ignore) 
  + DE(pos + normal * coeffs[2], ignore) 
  + DE(pos + normal * coeffs[3], ignore);
  const float ambient = max(0.001f, alpha / betta * 0.1f);

  return std::max(ambient, dot(normal, light_dir) * (1 - shadowed));
}

void RayMarcherExample::kernel2D_RayMarch(uint32_t* out_color, uint32_t width, uint32_t height) 
{
  for(uint32_t y=0;y<height;y++) 
  {
    for(uint32_t x=0;x<width;x++) 
    {
      const float unit_pixel_size = 1.f / float(width + height); 
      float3 rayDir = EyeRayDir((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height), m_worldViewProjInv); 
      float3 rayPos = float3(0.0f, 0.0f, 0.0f);

      transform_ray3f(m_worldViewInv, &rayPos, &rayDir);

      const uint32_t MAX_ITER = 255;
      const float MAX_DIST = 10.f;
      const float eps = 0.00001f;

      float4 resColor(0.0f);
      float prev_dist = 1.f / eps;
      const float3 origin = rayPos;
      float3 prev_pos = rayPos;
      uint32_t i = 0;
      for(i = 0; i < MAX_ITER; ++i) {
        const float dist = DE(rayPos, resColor);

        if (dist > MAX_DIST) {
          resColor = float4(0.f);
          break;
        }

        if (dist < eps) {
          const float3 normal = calculateNorm(prev_pos, eps);
          resColor *= calculateLight(rayPos, normal, light_dir, eps, MAX_ITER, MAX_DIST);
          break;
        }

        prev_pos = rayPos;
        prev_dist = dist;

        rayPos += rayDir * dist; 
      }

      if (i == MAX_ITER) {
        resColor = float4(0.f);
      }
      
      out_color[y*width+x] = RealColorToUint32(resColor);
    }
  }
}

void RayMarcherExample::RayMarch(uint32_t* out_color, uint32_t width, uint32_t height)
{ 
  auto start = std::chrono::high_resolution_clock::now();
  kernel2D_RayMarch(out_color, width, height);
  rayMarchTime = float(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count())/1000.f;
}  

void RayMarcherExample::GetExecutionTime(const char* a_funcName, float a_out[4])
{
  if(std::string(a_funcName) == "RayMarch")
    a_out[0] =  rayMarchTime;
}
