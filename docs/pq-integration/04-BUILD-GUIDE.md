# pqTox Build Guide

Complete instructions for building pqTox with PQ-enabled toxcore.

## Quick Start (Linux)

```bash
# 1. Build PQ-enabled toxcore
cd /home/waterfall/c-toxcore-pq
mkdir _build && cd _build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
sudo ldconfig

# 2. Build pqTox
cd /home/waterfall/pqTox
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
make -j$(nproc)

# 3. Run
./qtox
```

---

## Detailed Build Instructions

### Prerequisites

#### Ubuntu/Debian

```bash
# Qt and build tools
sudo apt update
sudo apt install -y \
    build-essential cmake git pkg-config \
    qtbase5-dev qttools5-dev qttools5-dev-tools \
    qtmultimedia5-dev libqt5svg5-dev libqt5opengl5-dev

# Audio/Video
sudo apt install -y \
    libavcodec-dev libavdevice-dev libavformat-dev \
    libavutil-dev libswscale-dev \
    libopus-dev libvpx-dev

# Database
sudo apt install -y libsqlcipher-dev

# Other dependencies
sudo apt install -y \
    libexif-dev libqrencode-dev libsodium-dev \
    libglib2.0-dev libgtk-3-dev
```

#### Fedora

```bash
sudo dnf install -y \
    cmake gcc-c++ git pkgconfig \
    qt5-qtbase-devel qt5-qttools-devel qt5-qtsvg-devel \
    qt5-qtmultimedia-devel \
    ffmpeg-devel opus-devel libvpx-devel \
    sqlcipher-devel libsodium-devel \
    libexif-devel qrencode-devel
```

---

### Step 1: Build PQ-Enabled libsodium

The default libsodium doesn't have ML-KEM. Build from source:

```bash
cd /tmp
git clone https://github.com/jedisct1/libsodium.git
cd libsodium
git checkout master

autoreconf -fi
./configure --prefix=/usr/local
make -j$(nproc)
sudo make install
sudo ldconfig

# Verify ML-KEM support
grep -q "crypto_kem_mlkem768" /usr/local/include/sodium.h && \
    echo "✓ ML-KEM-768 available" || \
    echo "✗ ML-KEM NOT available"
```

---

### Step 2: Build PQ-Enabled toxcore

```bash
cd /home/waterfall/c-toxcore-pq

# Initialize submodules
git submodule update --init

# Configure
mkdir -p _build && cd _build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DAUTOTEST=ON

# Build
make -j$(nproc)

# Test PQ functionality
./auto_tests/auto_crypto_pq_test

# Install
sudo make install
sudo ldconfig

# Verify installation
pkg-config --modversion toxcore
nm -D /usr/local/lib/libtoxcore.so | grep -q identity_status && \
    echo "✓ PQ identity API available" || \
    echo "✗ PQ API NOT available"
```

---

### Step 3: Build pqTox

```bash
cd /home/waterfall/pqTox

# Clean any previous builds
rm -rf build
mkdir build && cd build

# Configure with PQ toxcore
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/usr/local \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# Check for PQ detection in output:
# -- Found PQ-enabled toxcore

# Build
make -j$(nproc)

# Test
ctest --output-on-failure

# Install (optional)
sudo make install
```

---

## Windows Cross-Compilation

### Using the Native Script

```bash
cd /home/waterfall/c-toxcore-pq

# Build toxcore for Windows
./scripts/build-windows-native.sh

# Output in _win_native/x86_64/
ls _win_native/x86_64/lib/
# libtoxcore.a  libsodium.a
```

### Building pqTox for Windows

pqTox requires Qt which is complex to cross-compile. Options:

1. **MXE (M cross environment)**
   ```bash
   # Install MXE
   git clone https://github.com/mxe/mxe.git
   cd mxe
   make qt5

   # Use MXE to build pqTox
   export PATH=/path/to/mxe/usr/bin:$PATH
   cd /home/waterfall/pqTox
   mkdir build-win && cd build-win
   x86_64-w64-mingw32.static-cmake ..
   make -j$(nproc)
   ```

2. **Use pre-built Qt for MinGW**
   - Download Qt MinGW from qt.io
   - Set CMAKE_PREFIX_PATH to Qt installation

3. **Native Windows build**
   - Build on Windows with MSVC + vcpkg
   - See qTox's INSTALL.md for Windows instructions

---

## macOS Build

```bash
# Install dependencies via Homebrew
brew install cmake qt@5 ffmpeg opus libvpx \
    sqlcipher libsodium qrencode libexif

# Build libsodium with ML-KEM (if not in Homebrew version)
cd /tmp
git clone https://github.com/jedisct1/libsodium.git
cd libsodium && ./autogen.sh
./configure --prefix=/usr/local && make && sudo make install

# Build toxcore
cd /home/waterfall/c-toxcore-pq
mkdir _build && cd _build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)"
make -j$(sysctl -n hw.ncpu)
sudo make install

# Build pqTox
cd /home/waterfall/pqTox
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5);/usr/local"
make -j$(sysctl -n hw.ncpu)

# Create app bundle
make package
```

---

## Docker Build

For reproducible builds:

```dockerfile
# Dockerfile.pqtox
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    qtbase5-dev qttools5-dev qtmultimedia5-dev \
    libqt5svg5-dev libopus-dev libvpx-dev \
    libsqlcipher-dev libsodium-dev libexif-dev \
    libqrencode-dev autoconf automake libtool

# Build libsodium with ML-KEM
WORKDIR /build
RUN git clone https://github.com/jedisct1/libsodium.git && \
    cd libsodium && autoreconf -fi && \
    ./configure && make -j$(nproc) && make install && ldconfig

# Copy and build toxcore
COPY c-toxcore-pq /build/c-toxcore-pq
WORKDIR /build/c-toxcore-pq/_build
RUN cmake .. && make -j$(nproc) && make install && ldconfig

# Copy and build pqTox
COPY pqTox /build/pqTox
WORKDIR /build/pqTox/build
RUN cmake .. && make -j$(nproc)

# Output binary
CMD ["cp", "/build/pqTox/build/qtox", "/output/"]
```

Build with:
```bash
docker build -t pqtox-builder -f Dockerfile.pqtox .
docker run -v $(pwd)/output:/output pqtox-builder
```

---

## Verification

### Check PQ Support is Compiled

```bash
# Method 1: Check cmake output
grep "PQ-enabled" /home/waterfall/pqTox/build/CMakeCache.txt

# Method 2: Check binary symbols
nm /home/waterfall/pqTox/build/qtox | grep -i pq

# Method 3: Check at runtime
./qtox --version  # Should mention PQ if compiled with support
```

### Run Tests

```bash
cd /home/waterfall/pqTox/build

# All tests
ctest --output-on-failure

# Specific test
ctest -R toxid --output-on-failure
```

---

## Troubleshooting

### "Qt5 not found"

```bash
# Ubuntu
sudo apt install qtbase5-dev

# Or specify Qt path
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt5
```

### "toxcore not found"

```bash
# Check pkg-config
pkg-config --cflags --libs toxcore

# If not found, set PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

### "libsodium doesn't have ML-KEM"

Your system libsodium is too old. Build from git master:
```bash
cd /tmp
git clone https://github.com/jedisct1/libsodium.git
cd libsodium && ./autogen.sh
./configure --prefix=/usr/local
make && sudo make install && sudo ldconfig
```

### "Undefined reference to tox_friend_get_identity_status"

The toxcore you're linking against doesn't have PQ support. Rebuild c-toxcore-pq and ensure it's installed to the right prefix:
```bash
cd /home/waterfall/c-toxcore-pq/_build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make && sudo make install && sudo ldconfig
```

---

## Build Matrix

| Platform | Toolchain | Status | Notes |
|----------|-----------|--------|-------|
| Linux x64 | GCC 11+ | ✅ Supported | Primary development platform |
| Linux ARM64 | GCC 11+ | ✅ Supported | Raspberry Pi, etc. |
| Windows x64 | MinGW-w64 | 🚧 WIP | Cross-compile from Linux |
| Windows x64 | MSVC | 📋 Planned | Native Windows build |
| macOS x64 | Clang | ✅ Supported | Via Homebrew |
| macOS ARM64 | Clang | ✅ Supported | Apple Silicon |
