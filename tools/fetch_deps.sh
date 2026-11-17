#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Velora Engine — dependency bootstrap.
#
# Provisions everything the engine needs into ./build (git-ignored):
#   build/deps/    source checkouts: Vulkan-Headers, Vulkan-Loader, glslang,
#                  GLM, stb
#   build/prefix/  installed Vulkan headers + loader (libvulkan.so)
#   build/icd/     SwiftShader software Vulkan ICD (for headless machines
#                  without a GPU; skip this if you have a real driver)
#
# After this script, build with:
#   cmake -S . -B build/engine -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/engine
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS="$ROOT/build/deps"
PREFIX="$ROOT/build/prefix"
ICD="$ROOT/build/icd"
export PATH="$HOME/.local/bin:$PATH"

SDK_TAG="vulkan-sdk-1.3.296.0"

clone() { # repo dir tag
  if [ ! -d "$DEPS/$2" ]; then
    git clone --depth 1 --branch "$3" "https://github.com/$1.git" "$DEPS/$2"
  fi
}

echo "==> toolchain"
if ! command -v cmake >/dev/null 2>&1; then
  pip3 install --user --break-system-packages cmake ninja
fi
if ! command -v pkg-config >/dev/null 2>&1; then
  # Vulkan-Loader asks for pkg-config even with WSI disabled; a stub suffices.
  printf '#!/bin/sh\nexit 0\n' > "$HOME/.local/bin/pkg-config"
  chmod +x "$HOME/.local/bin/pkg-config"
fi

echo "==> fetching sources"
mkdir -p "$DEPS"
clone KhronosGroup/Vulkan-Headers Vulkan-Headers "$SDK_TAG"
clone KhronosGroup/Vulkan-Loader  Vulkan-Loader  "$SDK_TAG"
clone KhronosGroup/glslang        glslang        "$SDK_TAG"
clone g-truc/glm                  glm            1.0.1
clone nothings/stb                stb            master

echo "==> installing Vulkan-Headers"
cmake -S "$DEPS/Vulkan-Headers" -B "$DEPS/vh-build" -G Ninja \
      -DCMAKE_INSTALL_PREFIX="$PREFIX" > /dev/null
cmake --build "$DEPS/vh-build" --target install > /dev/null

if [ ! -f "$PREFIX/lib/libvulkan.so" ]; then
  echo "==> building Vulkan-Loader"
  cmake -S "$DEPS/Vulkan-Loader" -B "$DEPS/vl-build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DVULKAN_HEADERS_INSTALL_DIR="$PREFIX" -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DBUILD_TESTS=OFF -DUPDATE_DEPS=OFF \
        -DBUILD_WSI_XCB_SUPPORT=OFF -DBUILD_WSI_XLIB_SUPPORT=OFF \
        -DBUILD_WSI_WAYLAND_SUPPORT=OFF -DBUILD_WSI_DIRECTFB_SUPPORT=OFF \
        -DUSE_GAS=OFF > /dev/null
  cmake --build "$DEPS/vl-build" --target install > /dev/null
fi

echo "==> fetching SwiftShader software Vulkan ICD"
if [ ! -f "$ICD/package/linux-x64/libvk_swiftshader.so" ]; then
  mkdir -p "$ICD"
  curl -fSL -o "$ICD/ssvk.tgz" \
    "https://registry.npmjs.org/@onirenaud/swiftshader-vulkan/-/swiftshader-vulkan-0.1.0.tgz"
  tar xzf "$ICD/ssvk.tgz" -C "$ICD"
fi

echo
echo "Dependencies ready. Build & run:"
echo "  cmake -S . -B build/engine -G Ninja -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build/engine -j"
echo "  export VK_ICD_FILENAMES=\"$ICD/package/linux-x64/vk_swiftshader_icd.json\""
echo "  export LD_LIBRARY_PATH=\"$PREFIX/lib:\$LD_LIBRARY_PATH\""
echo "  ./build/engine/velora showcase"
