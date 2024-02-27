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
  const float R = 1.5f;
  return std::max(0.0f, length(pos) - R);
}

static inline float DE_tetrahedron(float3 pos) {
  const uint32_t Iterations = 10;
  const float Scale = 2.0f;

  const float figSize = 1.5f;
  const float3 a1 = figSize * float3(1,1,1);
	const float3 a2 = figSize * float3(-1,-1,1);
	const float3 a3 = figSize * float3(1,-1,-1);
	const float3 a4 = figSize * float3(-1,1,-1);

	float3 c;
	int n = 0;

	float dist, d;
	while (n < Iterations) {
		c = a1; 
    dist = length(pos-a1);

	  d = length(pos-a2); if (d < dist) { c = a2; dist=d; }
		d = length(pos-a3); if (d < dist) { c = a3; dist=d; }
		d = length(pos-a4); if (d < dist) { c = a4; dist=d; }

		pos = Scale*pos-c*(Scale-1.0);
		n++;
	}
	return length(pos) * pow(Scale, float(-n));
}

void RayMarcherExample::kernel2D_RayMarch(uint32_t* out_color, uint32_t width, uint32_t height) 
{
  for(uint32_t y=0;y<height;y++) 
  {
    for(uint32_t x=0;x<width;x++) 
    {
      float3 rayDir = EyeRayDir((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height), m_worldViewProjInv); 
      float3 rayPos = float3(0.0f, 0.0f, 0.0f);

      transform_ray3f(m_worldViewInv, &rayPos, &rayDir);

      const uint32_t MAX_ITER = 255;
      const float3 light_dir{0.5f, 0.5f, 0.5f};
      const float eps = 0.01f;

      float4 resColor(0.0f);
      float prev_dist = 1.f / eps;
      float3 prev_pos = rayPos;
      for(uint32_t i = 0; i < MAX_ITER; ++i) {
        const float dist = DE_tetrahedron(rayPos);

        if (dist < eps) {
          resColor = {1.f, 1.f, 1.f, 1.f};

          // calculate normal
          const float dx = DE_tetrahedron(prev_pos + float3(eps, 0.f, 0.f)) - prev_dist;
          const float dy = DE_tetrahedron(prev_pos + float3(0.f, eps, 0.f)) - prev_dist;
          const float dz = DE_tetrahedron(prev_pos + float3(0.f, 0.f, eps)) - prev_dist;
          const float3 normal = normalize(float3{dx, dy, dz});

          resColor *= std::max(0.1f, dot(normal, light_dir));
          break;
        }

        prev_pos = rayPos;
        prev_dist = dist;

        rayPos += rayDir * dist; 
        if (length(rayPos) > 10.0) {
          break;
        }
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
