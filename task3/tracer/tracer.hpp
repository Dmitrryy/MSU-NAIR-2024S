#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "LiteMath.h"
#include "siren.hpp"
using namespace LiteMath;

#define SIREN_MAX_LAYER_SIZE 256

class RayMarcherExample // : public IRenderAPI
{
public:
  RayMarcherExample(uint32_t n_layers, uint32_t layer_size,
                    const std::string &weights_path) {
    const float4x4 view = lookAt(float3(0, 1.5, -3), float3(0, 0, 0),
                                 float3(0, 1, 0)); // pos, look_at, up
    const float4x4 proj = perspectiveMatrix(90.0f, 1.0f, 0.1f, 100.0f);
    m_worldViewInv = inverse4x4(view);
    m_worldViewProjInv = inverse4x4(proj);
    light_dir = normalize(float3{0.3f, 0.5f, 0.5f});

    // SIREN
    m_hidden_layer_size = layer_size;
    m_hidden_layers_count = n_layers;
    assert(m_hidden_layers_count >= 2);
    assert(m_hidden_layer_size <= SIREN_MAX_LAYER_SIZE && "I am so sorry");

    m_siren_hidden_layers_data =
        loadWeights(weights_path, n_layers, layer_size);
    std::cout << "Loaded weighs params: " << m_siren_hidden_layers_data.size()
              << std::endl;

    // DEBUG
    siren_dump_arc(n_layers, layer_size);
    // END DEBUG
  }

  void SetWorldViewMatrix(const float4x4 &a_mat) {
    m_worldViewInv = inverse4x4(a_mat);
  }

  virtual void kernel2D_RayMarch(uint32_t *out_color, uint32_t width,
                                 uint32_t height);
  virtual void RayMarch(uint32_t *out_color [[size("width*height")]],
                        uint32_t width, uint32_t height);

  virtual void CommitDeviceData() {} // will be overriden in generated class
  virtual void UpdateMembersPlainData() {
  } // will be overriden in generated class (optional function)
  // virtual void UpdateMembersVectorData() {}                              //
  // will be overriden in generated class (optional function) virtual void
  // UpdateMembersTexureData() {}                              // will be
  // overriden in generated class (optional function)
  virtual void
  GetExecutionTime(const char *a_funcName,
                   float a_out[4]); // will be overriden in generated class

  float DE(float3 pos, float4 &color, bool &is_mirror);
  float3 calculateNorm(float3 pos, float eps);
  float calculateLight(float3 pos, float3 normal, float3 light_dir, float eps,
                       int MAX_ITER, float MAX_DIST);
  float4 calculateReflection(float3 pos, float3 eye_dir, float3 normal,
                             float3 light_dir, float eps, int MAX_ITER,
                             float MAX_DIST);

protected:
  float4x4 m_worldViewProjInv;
  float4x4 m_worldViewInv;
  float rayMarchTime;
  float3 light_dir;

  // SIREN
  // NOTE: kernel slicer doesn't support custom structures?
  //=------------------------------------------------------
  uint32_t m_hidden_layer_size = 256;
  uint32_t m_hidden_layers_count = 5;
  float m_siren_w0 = 30;
  // 1:   3 x m_hidden_layer_size
  // n-2: m_hidden_layers_count x m_hidden_layer_size x m_hidden_layer_size
  // 1:   m_hidden_layer_size x 1
  std::vector<float> m_siren_hidden_layers_data;

public:
  float siren_forward(float3 pos);

protected:
  // matrix methods
  void mat_mul(const float A[SIREN_MAX_LAYER_SIZE], uint32_t a_n, uint32_t a_m,
               uint32_t B_offset, uint32_t b_n, uint32_t b_m, float C[SIREN_MAX_LAYER_SIZE]) {
// #pragma omp parallel for
    for (uint32_t i = 0; i < a_n; ++i) {
      for (uint32_t j = 0; j < b_m; ++j) {
        const uint32_t res_idx = i * a_n + b_m;
        float cur_c = 0;
        for (uint32_t k = 0; k < a_m; ++k) {
          cur_c += A[i * a_n + k] *
                   m_siren_hidden_layers_data[k * a_m + j + B_offset];
        }
        C[res_idx] = cur_c;
      }
    }
  }

  void mat_mul_2(uint32_t A_offset, uint32_t a_n, uint32_t a_m,
                 const float B[SIREN_MAX_LAYER_SIZE], uint32_t b_n, uint32_t b_m, float C[SIREN_MAX_LAYER_SIZE]) {
// #pragma omp parallel for
    for (uint32_t i = 0; i < a_n; ++i) {
      for (uint32_t j = 0; j < b_m; ++j) {
        const uint32_t res_idx = i * b_m + j;
        float cur_c = 0;
        for (uint32_t k = 0; k < a_m; ++k) {
          uint32_t layer_elem_idx = i * a_m + k + A_offset;
          uint32_t B_idx = k * b_m + j;
          cur_c += m_siren_hidden_layers_data[layer_elem_idx] * B[B_idx];
        }
        C[res_idx] = cur_c;
      }
    }
  }

  void mat_add(const float A[SIREN_MAX_LAYER_SIZE], uint32_t a_n, uint32_t a_m,
               uint32_t B_offset, float C[SIREN_MAX_LAYER_SIZE]) {
    for (uint32_t i = 0; i < a_n * a_m; ++i) {
      C[i] = A[i] + m_siren_hidden_layers_data[B_offset + i];
    }
  }

  void siren_activation(float A[SIREN_MAX_LAYER_SIZE], uint32_t a_n, uint32_t a_m) {
    for (uint32_t i = 0; i < a_n * a_m; ++i) {
      A[i] = sin(m_siren_w0 * A[i]);
    }
  }
};
