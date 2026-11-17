cmake_minimum_required(VERSION 3.20)
project(VeloraEngine VERSION 0.1.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Velora Engine — an NPR-first game engine written in modern C++20 + Vulkan.
#
# Dependencies are provisioned by tools/fetch_deps.sh into build/deps and
# build/prefix (Vulkan-Headers, Vulkan-Loader, glslang, GLM, stb).
# ---------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VELORA_DEPS_DIR "${CMAKE_SOURCE_DIR}/build/deps" CACHE PATH "Third-party source checkouts")
set(VELORA_PREFIX   "${CMAKE_SOURCE_DIR}/build/prefix" CACHE PATH "Prefix with Vulkan-Headers/Loader installed")

find_path(VELORA_VULKAN_INCLUDE_DIR vulkan/vulkan.h
  PATHS "${VELORA_PREFIX}/include" NO_DEFAULT_PATH REQUIRED)
find_library(VELORA_VULKAN_LIBRARY NAMES vulkan libvulkan.so.1
  PATHS "${VELORA_PREFIX}/lib" NO_DEFAULT_PATH REQUIRED)

# ---- glslang: runtime GLSL -> SPIR-V shader compilation --------------------
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER      OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL             OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT              OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST            OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_JS       OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL          OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING           OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL    ON  CACHE BOOL "" FORCE)
add_subdirectory("${VELORA_DEPS_DIR}/glslang" "${CMAKE_BINARY_DIR}/glslang")

# ---- engine executable ------------------------------------------------------
add_executable(velora
  src/main.cpp
  src/vk/VulkanContext.cpp
  src/vk/Resources.cpp
  src/vk/ShaderLibrary.cpp
  src/vk/Pipeline.cpp
  src/render/Mesh.cpp
  src/render/Scene.cpp
  src/render/Renderer.cpp
)

target_include_directories(velora PRIVATE
  src
  "${VELORA_VULKAN_INCLUDE_DIR}"
  "${VELORA_DEPS_DIR}/glm"
  "${VELORA_DEPS_DIR}/stb"
  "${VELORA_DEPS_DIR}/glslang"
)

target_link_libraries(velora PRIVATE
  glslang
  SPIRV
  "${VELORA_VULKAN_LIBRARY}"
  ${CMAKE_DL_LIBS}
  pthread
)

target_compile_definitions(velora PRIVATE
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(velora PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Keep a copy of the GLSL shaders next to the binary as well.
add_custom_command(TARGET velora POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:velora>/shaders")
