# Distance-aided Ray Marching

Task: visualization of a scene whose objects are represented as a function of distance with sign (SDF).

## TODO

- [x] 3D Sierpinski triangle (CPU)
- [x] Transfer to GPU
- [ ] Supersampling
- [ ] Shadows
- [x] Settings for camera position
- [ ] Reflection
- [ ] Ambient Occlusion
- [ ] Unique color for each object
- [x] kernel_slicer usage

## kernel slicer

Vulkan kode + kernels generation:
```
<kslicer-build-dir>/kslicer <cur-src-dir>/tracer/tracer.cpp -mainClass RayMarcherExample -shaderCC glsl -pattern ipv -DKERNEL_SLICER -v -I<cur-build-dir>/_deps/litemath-src/ ignore -I<kslicer-rep-path>/kernel_slicer/TINYSTL ignore -stdlibfolder <kslicer-rep-path>/kernel_slicer/TINYSTL
```

For example:
```
./cmake-build-debug/kslicer /home/ddrozdov/Documents/repos/MSU-NAIR-2024S/task1/tracer/tracer.cpp -mainClass RayMarcherExample -shaderCC glsl -pattern ipv -DKERNEL_SLICER -v -I/home/ddrozdov/Documents/repos/MSU-NAIR-2024S/task1/build-gpu/_deps/litemath-src/ ignore -I/home/ddrozdov/Documents/repos/kernel_slicer/TINYSTL ignore -stdlibfolder /home/ddrozdov/Documents/repos/kernel_slicer/TINYSTL
```