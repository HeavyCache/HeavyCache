# HeavyCache

### Requirements

- CPU should support AVX2 instruction set.
- cmake >= 3.2
- g++ (MSVC is not supported currently.)

### How to build

The project is built upon [cmake](https://cmake.org/). You can use the following commands to build and run.

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ../
./demo
```
