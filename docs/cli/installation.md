# Installation

## Option 1 - pip (recommended)

The easiest way. The Python package ships a pre-built `novasvg` binary
alongside the Python extension, so no compiler or CMake is needed.

```bash
pip install novasvg
```

After install, `novasvg` is immediately available in your shell:

```bash
novasvg --version
```

## Option 2 - build with CMake

If you need a custom build configuration or are on a platform without
a pre-built wheel, you can build from source:

```bash
git clone https://github.com/MohammadRaziei/novasvg.git
cd novasvg
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Install to system (adds `novasvg` to PATH)
sudo cmake --install build
```

Requires CMake 3.19+ and a C++17 compiler (gcc / clang / MSVC).
