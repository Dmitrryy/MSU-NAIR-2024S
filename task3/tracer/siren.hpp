#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "LiteMath.h"
using namespace LiteMath;

static uint32_t getLayerColsSize_A(uint32_t layer, uint32_t n_layers,
                                   uint32_t layer_size) {
  if (layer == 0) {
    return 3;
  }
  return layer_size;
}

static uint32_t getLayerLinesSize_A(uint32_t layer, uint32_t n_layers,
                                    uint32_t layer_size) {
  if (layer == n_layers - 1) {
    return 1;
  }
  return layer_size;
}

static uint32_t getLayerSize_A(uint32_t layer, uint32_t n_layers,
                               uint32_t layer_size) {
  return getLayerLinesSize_A(layer, n_layers, layer_size) *
         getLayerColsSize_A(layer, n_layers, layer_size);
}

static uint32_t getLayerStartIdx_A(uint32_t layer, uint32_t n_layers,
                                   uint32_t layer_size) {
  if (layer == 0) {
    return 0;
  } else if (layer == n_layers - 1) {
    return (n_layers - 2) * (layer_size * layer_size + layer_size) +
           getLayerSize_A(0, n_layers, layer_size) + layer_size;
  }
  // assert(layer < n_layers);
  return (layer - 1) * (layer_size * layer_size + layer_size) +
         getLayerSize_A(0, n_layers, layer_size) + layer_size;
}

static uint32_t getLayerStartIdx_b(uint32_t layer, uint32_t n_layers,
                                   uint32_t layer_size) {
  return getLayerStartIdx_A(layer, n_layers, layer_size) +
         getLayerSize_A(layer, n_layers, layer_size);
}

static void siren_dump_arc(uint32_t n_layers, uint32_t layer_size) {
  for (uint32_t i = 0; i < n_layers; ++i) {
    std::cout << "Dense input shape ("
              << getLayerColsSize_A(i, n_layers, layer_size)
              << ") output shape ("
              << getLayerLinesSize_A(i, n_layers, layer_size)
              << "), start idx in data: "
              << getLayerStartIdx_A(i, n_layers, layer_size) << std::endl;
  }
}

// NOTE: kernel slicer doesn't support custom structures in the kernel
//       so, implemented "C" style
static std::vector<float> loadWeights(const std::string &path_name,
                                      uint32_t n_layers, uint32_t layer_size) {
  // open the file:
  std::ifstream file(path_name, std::ios::binary | std::ios::in);
  file.seekg(0, std::ios::end);
  std::cout << "Weights file size: " << file.tellg() << std::endl;
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> contents((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

  assert(contents.size() % 4 == 0);
  std::vector<float> res(contents.size() / 4);
  std::copy(contents.begin(), contents.end(),
            reinterpret_cast<uint8_t *>(res.data()));
  return res;
}

static std::pair<std::vector<float3>, std::vector<float>>
loadTest(const std::string &path_name) {
  // open the file:
  std::ifstream file(path_name, std::ios::binary | std::ios::in);
  file.seekg(0, std::ios::end);
  std::cout << "Test file size: " << file.tellg() << std::endl;
  file.seekg(0, std::ios::beg);
  // std::vector<uint8_t> contents((std::istreambuf_iterator<char>(file)),
  //                               std::istreambuf_iterator<char>());

  uint32_t N = 0;
  file.read(reinterpret_cast<char *>(&N), sizeof(N));
  std::cout << "N: " << N << std::endl;

  std::vector<float3> points(N);
  file.read(reinterpret_cast<char *>(points.data()), N * 12);

  std::vector<float> distances(N);
  file.read(reinterpret_cast<char *>(distances.data()), N * 4);

  return {points, distances};
}

// matrix operations
//=----------------------------------------------------------------------------
static uint32_t mat_get_elem_id_l(uint32_t line, uint32_t col, uint32_t a_n,
                                  uint32_t a_m) {
  return line * a_n + col;
}

static inline void mat_mul(float A[3], const std::vector<float> &B,
                           uint32_t a_n, uint32_t a_m, uint32_t b_n,
                           uint32_t b_m, float C[256]) {
  for (uint32_t i = 0; i < a_n; ++i) {
    for (uint32_t j = 0; j < b_m; ++j) {
      const uint32_t res_idx = i * a_n + b_m;
      float cur_c = 0;
      for (uint32_t k = 0; k < a_m; ++k) {
        cur_c += A[i * a_n + k] * B[k * a_m + j];
      }
      C[res_idx] = cur_c;
    }
  }
}
//=----------------------------------------------------------------------------

static void siren_forward(float *siren_data, uint32_t layer_size,
                          uint32_t layer, float *x_data, uint32_t x_lines,
                          uint32_t x_cols) {
  //
}
//=----------------------------------------------------------------------------
