# Neural SDF

The purpose of the assignment is to become familiar with the use of neural networks in practice.
for representing objects in computer graphics.
Main goals:
* Implementation of forward and backward pass of a neural network
(Multi-Layered Perceptron) without using third party
libraries;
* Using a neural network to approximate the distance function
to an arbitrary object.


## TODO

- [ ] SIREN (10 points)
- [ ] Visualization (5 points)
- [ ] SIREN training (10 points)
- [ ] Forward with GPU (5 points)
- [ ] Backward with GPU (10 points)

## Build

### CPU

```
mkdir build && cd build
```
```
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
```
```
cmake --build . -j8
```
launch:
```
./tracer_cpu
```
### GPU
This will download, build and use kernel_slicer automatically, so no addition actions required.

```
mkdir build && cd build
```
```
cmake -GNinja -DBUILD_WITH_KERNEL_SLICER=ON -DCMAKE_BUILD_TYPE=Release ..
```
```
cmake --build . -j8
```
launch:
```
./tracer_gpu
```
### GPU (Legacy)
Original way to build with kernel slicer.
1. clone and build kernel_slicer (https://github.com/Ray-Tracing-Systems/kernel_slicer) in some directory
2. find and use VS Code config 'Launch (LiteRF)' in 'kernel_slicer/.vscode/launch.json':
   * you need to change all paths to your LiteRF sources
   * launch kernel_slicer with this config
   * you can make you own command line script if don't like VS Code configs
3. build shaders:
   * cd tracer/shaders_generated && bash build.sh
4. use Cmake to build project with 'USE_VULKAN' flag == 'ON':
   * mkdir cmake-build-release && cd cmake-build-release
   * cmake -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=ON ..
   * make -j 8
   * Note that the program will seek for 'tracer/shaders_generated/kernel2D_RayMarch.comp.spv' 

## Performance results

### Setup

* System:
  * Kernel: 6.5.0-21-generic x86_64 bits: 64
    * Distro: Ubuntu 24
* Machine:
  * Type: Laptop System: ASUSTeK product: Vivobook_ASUSLaptop X3500PC_K3500PC
* CPU: 11th Gen Intel Core i7-11370H
* Graphics:
  * Device-1: Intel TigerLake-LP GT2 [Iris Xe Graphics]
  * Device-2: NVIDIA GA107M [GeForce RTX 3050 Mobile]

### Results

TODO
