#include <iostream>
#include <fstream>
#include <vector>
#include <memory>  // for shared pointers
#include <iomanip> // for std::fixed/std::setprecision

#include "tracer/tracer.hpp"
#include "Image2d.h"

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

#ifdef USE_VULKAN
#include "vk_context.h"
std::shared_ptr<RayMarcherExample> CreateRayMarcherExample_Generated(uint32_t n_layers, uint32_t layer_size, const std::string &weights_path, vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated); 
#endif

int main(int argc, const char** argv)
{
  CLI::App app{"Neuron SDF"};

  uint32_t n_layers = 0;
  app.add_option("n_layers", n_layers, "Number of layers")
    ->required();

  uint32_t layer_size = 0;
  app.add_option("layer_size", n_layers, "Size of hidden layers")
    ->required();

  std::string input{};
  app.add_option("weights", input, "Bin file with weights for model")
      ->required()
      ->check(CLI::ExistingFile);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  #ifndef NDEBUG
  bool enableValidationLayers = true;
  #else
  bool enableValidationLayers = false;
  #endif

  uint WIN_WIDTH  = 1024;
  uint WIN_HEIGHT = 1024;

  std::shared_ptr<RayMarcherExample> pImpl = nullptr;
  #ifdef USE_VULKAN
  bool onGPU = true; // TODO: you can read it from command line
  if(onGPU)
  {
    auto ctx = vk_utils::globalContextGet(enableValidationLayers, 1);
    pImpl    = CreateRayMarcherExample_Generated(n_layers, layer_size, input, ctx, WIN_WIDTH*WIN_HEIGHT);
  }
  else
  #else
  bool onGPU = false;
  #endif
    pImpl = std::make_shared<RayMarcherExample>(n_layers, layer_size, input);

  pImpl->CommitDeviceData();

  std::vector<uint> pixelData(WIN_WIDTH*WIN_HEIGHT);  

  for(int angleY = 0; angleY < 360; angleY += 30) 
  {
    float4x4 mRot    = rotate4x4Y(float(angleY)*DEG_TO_RAD);
    float4   camPos  = mRot*float4(0,0,-4,0) + float4(0,1.5f,0,1);              // rotate and than translate camera position
    float4x4 viewMat = lookAt(to_float3(camPos), float3(0,0,0), float3(0,1,0)); // pos, look_at, up  
    
    pImpl->SetWorldViewMatrix(viewMat);
    pImpl->UpdateMembersPlainData();                                            // copy all POD members from CPU to GPU in GPU implementation
    pImpl->RayMarch(pixelData.data(), WIN_WIDTH, WIN_HEIGHT);
  
    float timings[4] = {0,0,0,0};
    pImpl->GetExecutionTime("RayMarch", timings);

    std::stringstream strOut;
    if(onGPU)
      strOut << std::fixed << std::setprecision(2) << "out_gpu_" << angleY << ".bmp";
    else
      strOut << std::fixed << std::setprecision(2) << "out_cpu_" << angleY << ".bmp";
    std::string fileName = strOut.str();

    LiteImage::SaveBMP(fileName.c_str(), pixelData.data(), WIN_WIDTH, WIN_HEIGHT);

    std::cout << "angl = " << angleY << ", timeRender = " << timings[0] << " ms, timeCopy = " <<  timings[1] + timings[2] << " ms " << std::endl;
  }
  
  pImpl = nullptr;
  return 0;
}