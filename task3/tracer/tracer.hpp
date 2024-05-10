#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "LiteMath.h"
#include "siren.hpp"
using namespace LiteMath;

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
    m_siren_hidden_layers_data = loadWeights(weights_path, n_layers, layer_size);
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
};
