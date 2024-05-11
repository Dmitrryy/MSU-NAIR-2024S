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

- [x] SIREN (10 points)
- [x] Visualization (5 points)
- [ ] SIREN training (10 points)
- [x] Forward with GPU (5 points)
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

## Start

### Tests
1
```
./tracer_cpu_test 4 64 ../references/sdf1_weights.bin ../references/sdf1_test.bin
```
2
```
./tracer_cpu_test 6 256 ../references/sdf2_weights.bin ../references/sdf2_test.bin
```

### Visualization

1
```
./tracer_cpu 4 64 ../references/sdf1_weights.bin
```
or
```
./tracer_gpu 4 64 ../references/sdf1_weights.bin
```
2
```
./tracer_cpu 6 256 ../references/sdf2_weights.bin
```

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

ender times for `out_gpu_0.bmp` and  `out_cpu_0.bmp` from preview:

| Device          | Note              | Render Time | Copy Time (CPU <-> GPU) |
|-----------------|-------------------|-------------|-----------|
| CPU             |single-thread      | 13200 ms    | 0ms       |
| CPU             |8 threads (OpenMP) | 3300 ms     | 0ms       |
| Iris Xe         |                   | TODO        | TODO      |
| RTX 3050 Mobile |                   | TODO        | TODO      |

