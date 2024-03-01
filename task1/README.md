# Distance-aided Ray Marching

Task: visualization of a scene whose objects are represented as a function of distance with sign (SDF).

## TODO

- [x] 3D Sierpinski triangle (CPU)
- [x] Transfer to GPU
- [ ] Supersampling
- [x] Shadows
- [x] Settings for camera position
- [x] Reflection
- [x] Ambient Occlusion
- [x] Unique color for each object
- [x] kernel_slicer usage

![Cur result](imgs/out_gpu_300.bmp)

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

#### kernel slicer cmd line

```
<kslicer-build-dir>/kslicer <cur-src-dir>/tracer/tracer.cpp -mainClass RayMarcherExample -shaderCC glsl -pattern ipv -DKERNEL_SLICER -v -I<cur-build-dir>/_deps/litemath-src/ ignore -I<kslicer-rep-path>/kernel_slicer/TINYSTL ignore -stdlibfolder <kslicer-rep-path>/kernel_slicer/TINYSTL
```

For example:
```
./cmake-build-debug/kslicer /home/ddrozdov/Documents/repos/MSU-NAIR-2024S/task1/tracer/tracer.cpp -mainClass RayMarcherExample -shaderCC glsl -pattern ipv -DKERNEL_SLICER -v -I/home/ddrozdov/Documents/repos/MSU-NAIR-2024S/task1/build-gpu/_deps/litemath-src/ ignore -I/home/ddrozdov/Documents/repos/kernel_slicer/TINYSTL ignore -stdlibfolder /home/ddrozdov/Documents/repos/kernel_slicer/TINYSTL
```

## Performance results

### Setup

* System:
  * Kernel: 6.5.0-21-generic x86_64 bits: 64
    * Distro: Ubuntu 22.04.4 LTS (Jammy Jellyfish)
* Machine:
  * Type: Laptop System: ASUSTeK product: Vivobook_ASUSLaptop X3500PC_K3500PC
* CPU: 11th Gen Intel Core i7-11370H
* Graphics:
  * Device-1: Intel TigerLake-LP GT2 [Iris Xe Graphics]
  * Device-2: NVIDIA GA107M [GeForce RTX 3050 Mobile]

### Results
Render times for `out_gpu_300.bmp` and  `out_cpu_300.bmp` from preview:

| Device          | Note              | Render Time | Copy Time (CPU <-> GPU) |
|-----------------|-------------------|-------------|-----------|
| CPU             |single-thread      | 6300 ms     | 0ms       |
| CPU             |8 threads (OpenMP) | 1800 ms     | 0ms       |
| Iris Xe         |                   | 130 ms      | 22 ms     |
| RTX 3050 Mobile |                   | 73 ms       | 22 ms     |


