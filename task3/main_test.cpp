#include <fstream>
#include <iomanip> // for std::fixed/std::setprecision
#include <iostream>
#include <memory> // for shared pointers
#include <vector>

#include "Image2d.h"
#include "tracer/siren.hpp"
#include "tracer/tracer.hpp"

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

int main(int argc, const char **argv) {
  CLI::App app{"Neuron SDF"};

  uint32_t n_layers = 0;
  app.add_option("n_layers", n_layers, "Number of layers")->required();

  uint32_t layer_size = 0;
  app.add_option("layer_size", layer_size, "Size of hidden layers")->required();

  std::string input{};
  app.add_option("weights", input, "Bin file with weights for model")
      ->required()
      ->check(CLI::ExistingFile);

  std::string tests{};
  app.add_option("tests", tests, "Bin file with tests")
      ->required()
      ->check(CLI::ExistingFile);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  auto pImpl = std::make_shared<RayMarcherExample>(n_layers, layer_size, input);

  auto &&[points, distances] = loadTest(tests);

  size_t errors = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    float distance_pred = pImpl->siren_forward(points[i]);
    if (std::abs(distance_pred - distances[i]) > 0.0001) {
      std::cout << i << "/" << points.size() << " | Pred: " << distance_pred
                << ", reference: " << distances[i] << std::endl;
      ++errors;

      if (errors > 10) {
        break;
      }
    }
  }

  return 0;
}