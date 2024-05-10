#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

static uint32_t getLayerLinesSize_A(uint32_t layer, uint32_t n_layers,
                                    uint32_t layer_size) {
  if (layer == 0) {
    return layer_size * 4;
  } else if (layer == n_layers - 1) {
    return layer_size;
  }
  assert(layer < n_layers);
  return layer_size * layer_size;
}

static uint32_t getLayerSize_A(uint32_t layer, uint32_t n_layers,
                               uint32_t layer_size) {
  if (layer == 0) {
    return layer_size * 3;
  } else if (layer == n_layers - 1) {
    return layer_size;
  }
  assert(layer < n_layers);
  return layer_size * layer_size;
}

static uint32_t getLayerStartIdx_A(uint32_t layer, uint32_t n_layers,
                                   uint32_t layer_size) {
  if (layer == 0) {
    return 0;
  } else if (layer == n_layers - 1) {
    return (n_layers - 2) * (layer_size * layer_size + layer_size) +
           getLayerSize_A(0, n_layers, layer_size) + layer_size;
  }
  assert(layer < n_layers);
  return (layer - 1) * (layer_size * layer_size + layer_size) +
         getLayerSize_A(0, n_layers, layer_size) + layer_size;
}

static uint32_t getLayerStartIdx_b(uint32_t layer, uint32_t n_layers,
                                   uint32_t layer_size) {
  return getLayerStartIdx_A(layer, n_layers, layer_size) +
         getLayerSize_A(layer, n_layers, layer_size);
}

// NOTE: kernel slicer doesn't support custom structures in the kernel
//       so, implemented "C" style
static std::vector<float> loadWeights(const std::string &path_name,
                                      uint32_t n_layers, uint32_t layer_size) {
  // open the file:
  std::ifstream file(path_name, std::ios::binary);
  std::vector<uint8_t> contents((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

  assert(contents.size() % 4 == 0);
  std::vector<float> res(contents.size() / 4);
  std::copy(contents.begin(), contents.end(),
            reinterpret_cast<uint8_t *>(res.data()));
  return res;
}

static void siren_forward(float *siren_data, uint32_t layer_size,
                          uint32_t layer, float *x_data, uint32_t x_lines,
                          uint32_t x_cols) {
  //
}
