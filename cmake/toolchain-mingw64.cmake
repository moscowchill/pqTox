# CMake toolchain file for cross-compiling pqTox to Windows using MinGW-w64
#
# Usage:
#   mkdir build-win64 && cd build-win64
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake \
#            -DCMAKE_PREFIX_PATH="/path/to/deps;/path/to/qt6"
#   make -j$(nproc)
#
# Prerequisites (Ubuntu/Debian):
#   sudo apt-get install mingw-w64 mingw-w64-tools

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Cross-compiler settings
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib CACHE FILEPATH "Ranlib")

# Target environment location
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# Use BOTH for packages to allow finding Qt6 components in non-sysroot paths
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# Windows-specific settings
set(WIN32 TRUE)
set(MINGW TRUE)

# Static linking for easier distribution
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Output file extensions
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

# pkg-config for cross-compilation
set(PKG_CONFIG_EXECUTABLE ${TOOLCHAIN_PREFIX}-pkg-config CACHE FILEPATH "pkg-config")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/${TOOLCHAIN_PREFIX}/lib/pkgconfig")

# Qt6 settings for cross-compilation
set(QT_HOST_PATH "" CACHE PATH "Path to host Qt installation for tools")
