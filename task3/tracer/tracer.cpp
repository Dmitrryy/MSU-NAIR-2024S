#include <chrono>
#include <string>
#include <vector>

#include "siren.hpp"
#include "tracer.hpp"

static inline float3 EyeRayDir(float x, float y, float4x4 a_mViewProjInv) {
  float4 pos = float4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 0.0f, 1.0f);
  pos = a_mViewProjInv * pos;
  pos /= pos.w;
  return normalize(to_float3(pos));
}

static inline void transform_ray3f(float4x4 a_mWorldViewInv, float3 *ray_pos,
                                   float3 *ray_dir) {
  float4 rayPosTransformed = a_mWorldViewInv * to_float4(*ray_pos, 1.0f);
  float4 rayDirTransformed = a_mWorldViewInv * to_float4(*ray_dir, 0.0f);

  (*ray_pos) = to_float3(rayPosTransformed);
  (*ray_dir) = to_float3(normalize(rayDirTransformed));
}

static inline uint32_t RealColorToUint32(float4 real_color) {
  float r = real_color[0] * 255.0f;
  float g = real_color[1] * 255.0f;
  float b = real_color[2] * 255.0f;
  float a = real_color[3] * 255.0f;

  uint32_t red = (uint32_t)r;
  uint32_t green = (uint32_t)g;
  uint32_t blue = (uint32_t)b;
  uint32_t alpha = (uint32_t)a;

  return red | (green << 8) | (blue << 16) | (alpha << 24);
}

static inline float DE_sphere(float3 pos) {
  const float R = 1.f;
  return std::max(0.0f, length(pos) - R);
}

inline float SDE_Parallelepiped(float3 point, float3 size) {
  float3 d = abs(point) - size;
  return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, float3(0.0f)));
}

float RayMarcherExample::DE(float3 pos, float4 &color, bool &is_mirror) {
  float result = 1 / 0.000000001f;
  pos /= 1.2f;

  // model return undefined values for points outside [-1, 1] box.
  if (abs(pos.x) >= 1.f || abs(pos.y) >= 1.f || abs(pos.z) >= 1.f) {
    return result;
  }

  // Neural
  is_mirror = false;
  color = float4(1.0);
  return siren_forward(pos);
}

float RayMarcherExample::siren_forward(float3 pos) {
  float X[SIREN_MAX_LAYER_SIZE];
  X[0] = pos.x;
  X[1] = pos.y;
  X[2] = pos.z;

  float X_tmp[SIREN_MAX_LAYER_SIZE];
  for (uint32_t layer = 0; layer < m_hidden_layers_count; ++layer) {
    const uint32_t layer_lines =
        getLayerLinesSize_A(layer, m_hidden_layers_count, m_hidden_layer_size);
    const uint32_t layer_cols =
        getLayerColsSize_A(layer, m_hidden_layers_count, m_hidden_layer_size);
    const uint32_t W_offset =
        getLayerStartIdx_A(layer, m_hidden_layers_count, m_hidden_layer_size);
    const uint32_t b_offset =
        getLayerStartIdx_b(layer, m_hidden_layers_count, m_hidden_layer_size);
    // W * X + b
    mat_mul_2(W_offset, layer_lines, layer_cols, X, layer_cols, 1, X_tmp);
    mat_add(X_tmp, layer_lines, 1, b_offset, X);
    if (layer != m_hidden_layers_count - 1) {
      siren_activation(X, layer_lines, 1);
    }
  }

  return X[0];
}

inline float3 RayMarcherExample::calculateNorm(float3 pos, float eps) {
  float4 ignore;
  bool ignore2;
  const float dx = DE(pos + float3(eps, 0.f, 0.f), ignore, ignore2) -
                   DE(pos - float3(eps, 0.f, 0.f), ignore, ignore2);
  const float dy = DE(pos + float3(0.f, eps, 0.f), ignore, ignore2) -
                   DE(pos - float3(0.f, eps, 0.f), ignore, ignore2);
  const float dz = DE(pos + float3(0.f, 0.f, eps), ignore, ignore2) -
                   DE(pos - float3(0.f, 0.f, eps), ignore, ignore2);
  return normalize(float3{dx, dy, dz});
}

inline float RayMarcherExample::calculateLight(float3 pos, float3 normal,
                                               float3 light_dir, float eps,
                                               int MAX_ITER, float MAX_DIST) {
  float3 shadowRayPos = pos + normal * eps;
  float3 shadowRayDir = light_dir;
  float4 ignore;
  bool ignore2;
  // bool shadowed = false;
  // for (uint32_t s = 0; s < MAX_ITER; ++s) {
  //   const float d = DE(shadowRayPos, ignore, ignore2);
  //   if (d < eps) {
  //     shadowed = true;
  //     break;
  //   }
  //   if (d > MAX_DIST) {
  //     break;
  //   }
  //   shadowRayPos += d * shadowRayDir;
  // }

  // Ambient occlusion
  // const float4 coeffs(0.1f, 0.1f, 0.25f, 0.4f);
  // const float betta = coeffs[0] + coeffs[1] + coeffs[2] + coeffs[3];
  // const float alpha = DE(pos + normal * coeffs[0], ignore, ignore2) +
  //                     DE(pos + normal * coeffs[1], ignore, ignore2) +
  //                     DE(pos + normal * coeffs[2], ignore, ignore2) +
  //                     DE(pos + normal * coeffs[3], ignore, ignore2);
  // const float ambient = max(0.0001f, alpha / betta * 0.1f);
  const float ambient = 0.1f;

  return max(ambient, dot(normal, light_dir)); // * (1 - shadowed));
}

inline float4 RayMarcherExample::calculateReflection(float3 pos, float3 eye_dir,
                                                     float3 normal,
                                                     float3 light_dir,
                                                     float eps, int MAX_ITER,
                                                     float MAX_DIST) {
  float3 reflectDir = reflect(eye_dir, normal);
  pos = pos + normal * eps;

  float4 color;
  bool ignore2;
  float3 prev_pos = pos;
  for (uint32_t s = 0; s < MAX_ITER; ++s) {
    const float d = DE(pos, color, ignore2);
    if (d < eps) {
      const float3 normal = calculateNorm(prev_pos, eps);
      color *= calculateLight(pos, normal, light_dir, eps, MAX_ITER, MAX_DIST);
      break;
    }
    if (d > MAX_DIST) {
      color = float4(0.f);
      break;
    }
    prev_pos = pos;
    pos += d * reflectDir;
  }
  return mix(float4(0.1, 0.12, 0.15, 1.f), color, 0.85f);
}

void RayMarcherExample::kernel2D_RayMarch(uint32_t *out_color, uint32_t width,
                                          uint32_t height) {
// #pragma omp parallel for
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      const float unit_pixel_size = 1.f / float(width + height);
      float3 rayDir =
          EyeRayDir((float(x) + 0.5f) / float(width),
                    (float(y) + 0.5f) / float(height), m_worldViewProjInv);
      float3 rayPos = float3(0.0f, 0.0f, 0.0f);

      transform_ray3f(m_worldViewInv, &rayPos, &rayDir);

      const uint32_t MAX_ITER = 255;
      const float MAX_DIST = 3.f;
      const float eps = 0.00001f;

      float4 resColor(0.0f);
      float prev_dist = 1.f / eps;
      const float3 origin = rayPos;
      float3 prev_pos = rayPos;
      uint32_t i = 0;
      bool is_mirror = false;
      for (i = 0; i < MAX_ITER; ++i) {
        const float dist = DE(rayPos, resColor, is_mirror);

        if (dist > MAX_DIST) {
          resColor = float4(0.f);
          break;
        }

        if (dist < eps) {
          const float3 normal = calculateNorm(prev_pos, eps);
          if (is_mirror) {
            resColor = calculateReflection(rayPos, rayDir, normal, light_dir,
                                           eps, MAX_ITER, MAX_DIST);
          } else {
            resColor *= calculateLight(rayPos, normal, light_dir, eps, MAX_ITER,
                                       MAX_DIST);
          }
          break;
        }

        prev_pos = rayPos;
        prev_dist = dist;

        rayPos += rayDir * dist;
      }

      if (i == MAX_ITER) {
        resColor = float4(0.f);
      }

      out_color[y * width + x] = RealColorToUint32(resColor);
    }
  }
}

void RayMarcherExample::RayMarch(uint32_t *out_color, uint32_t width,
                                 uint32_t height) {
  auto start = std::chrono::high_resolution_clock::now();
  kernel2D_RayMarch(out_color, width, height);
  rayMarchTime = float(std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::high_resolution_clock::now() - start)
                           .count()) /
                 1000.f;
}

void RayMarcherExample::GetExecutionTime(const char *a_funcName,
                                         float a_out[4]) {
  if (std::string(a_funcName) == "RayMarch")
    a_out[0] = rayMarchTime;
}
